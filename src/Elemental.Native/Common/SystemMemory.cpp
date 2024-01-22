#include "SystemMemory.h"
#include "SystemFunctions.h"
#include "SystemLogging.h"
#include "SystemPlatformFunctions.h"

#define MEMORYARENA_DEFAULT_SIZE 64 * 1024 * 1024

struct MemoryArenaPageInfo
{
    bool IsCommitted;
    uint32_t MinCommittedOffset;
    uint32_t MaxCommittedOffset;
};

struct MemoryArenaStorage
{
    uint8_t* CurrentPointer;
    size_t SizeInBytes;
    size_t HeaderSizeInBytes;
    uint8_t* CommitedMemoryPointer;
    bool IsCommitOperationInProgres;
    MemoryArenaPageInfo* PagesInfos;

    // Stack fields
    MemoryArena ExtraStorage;
    uint8_t Level;
    uint8_t MinAllocatedLevel;
};

thread_local MemoryArenaStorage* stackMemoryArenaStorage = nullptr;
size_t systemPageSizeInBytes = 0;

size_t ResizeToPageSizeMultiple(size_t sizeInBytes, size_t pageSizeInBytes)
{
    return (sizeInBytes + pageSizeInBytes - 1) & ~(pageSizeInBytes - 1);
}

#include <stdio.h>
MemoryArenaStorage* AllocateMemoryArenaStorage(size_t sizeInBytes)
{
    if (systemPageSizeInBytes == 0)
    {
        systemPageSizeInBytes = SystemPlatformGetPageSize();
    }

    auto pageInfosCount = SystemRoundUp((float)sizeInBytes / (float)systemPageSizeInBytes);
    auto headerSizeInBytes = sizeof(MemoryArenaStorage) + pageInfosCount * sizeof(MemoryArenaPageInfo);

    auto sizeResized = ResizeToPageSizeMultiple(headerSizeInBytes + sizeInBytes, systemPageSizeInBytes);
    auto storage = (MemoryArenaStorage*)SystemPlatformReserveMemory(sizeResized);

    auto headerResized = ResizeToPageSizeMultiple(headerSizeInBytes, systemPageSizeInBytes);
    SystemPlatformCommitMemory(storage, headerResized);

    storage->CurrentPointer = (uint8_t*)storage + headerSizeInBytes;
    storage->SizeInBytes = sizeInBytes;
    storage->HeaderSizeInBytes = headerSizeInBytes;
    storage->CommitedMemoryPointer = (uint8_t*)storage + headerResized;
    storage->IsCommitOperationInProgres = false;
    storage->PagesInfos = (MemoryArenaPageInfo*)((uint8_t*)storage + sizeof(MemoryArenaStorage));
    storage->ExtraStorage = {};
    storage->Level = 0;
    storage->MinAllocatedLevel = 255;

    printf("PageInfo Count: %llu, HeaderSize: %llu\n", pageInfosCount, storage->HeaderSizeInBytes);

    for (size_t i = 0; i < (size_t)pageInfosCount; i++)
    {
        // TODO: Init min and max
        if (headerResized / systemPageSizeInBytes > i)
        {
            storage->PagesInfos[i].IsCommitted = true;
            storage->PagesInfos[i].MinCommittedOffset = 0;
            storage->PagesInfos[i].MaxCommittedOffset = storage->HeaderSizeInBytes;
        }
        else 
        {
            storage->PagesInfos[i].IsCommitted = false;
        }
    }

    return storage;
}

MemoryArena GetStackWorkingMemoryArena(MemoryArena memoryArena)
{
    MemoryArena workingMemoryArena = memoryArena;

    if (memoryArena.Level != memoryArena.Storage->Level)
    {
        if (memoryArena.Storage->ExtraStorage.Storage == nullptr)
        {
            auto extraHandle = AllocateMemoryArenaStorage(MEMORYARENA_DEFAULT_SIZE);
            memoryArena.Storage->ExtraStorage = { extraHandle, 0 };
        }

        workingMemoryArena = memoryArena.Storage->ExtraStorage;
        memoryArena.Storage->MinAllocatedLevel = SystemMin(memoryArena.Storage->MinAllocatedLevel, memoryArena.Level);
    }

    return workingMemoryArena;
}

MemoryArena SystemAllocateMemoryArena()
{
    return SystemAllocateMemoryArena(MEMORYARENA_DEFAULT_SIZE);
}

MemoryArena SystemAllocateMemoryArena(size_t sizeInBytes)
{
    auto storage = AllocateMemoryArenaStorage(sizeInBytes);

    MemoryArena result = {};
    result.Storage = storage;
    result.Level = 0;
    return result;
}

void SystemFreeMemoryArena(MemoryArena memoryArena)
{
    SystemPlatformFreeMemory(memoryArena.Storage, memoryArena.Storage->HeaderSizeInBytes + memoryArena.Storage->SizeInBytes);
}

void SystemClearMemoryArena(MemoryArena memoryArena)
{
    SystemPopMemory(memoryArena, SystemGetMemoryArenaAllocatedBytes(memoryArena));
}

// TODO: Change this to infos and return also the committed pages and total pages
size_t SystemGetMemoryArenaAllocatedBytes(MemoryArena memoryArena)
{
    return memoryArena.Storage->CurrentPointer - (uint8_t*)memoryArena.Storage - memoryArena.Storage->HeaderSizeInBytes;
}

StackMemoryArena SystemGetStackMemoryArena()
{
    if (stackMemoryArenaStorage == nullptr)
    {
        stackMemoryArenaStorage = AllocateMemoryArenaStorage(MEMORYARENA_DEFAULT_SIZE);
    }
    
    stackMemoryArenaStorage->Level++;
    auto extraStorageAllocatedBytes = 0llu;

    if (stackMemoryArenaStorage->ExtraStorage.Storage != nullptr)
    {
        extraStorageAllocatedBytes = SystemGetMemoryArenaAllocatedBytes(stackMemoryArenaStorage->ExtraStorage);
    }

    MemoryArena memoryArena = {};
    memoryArena.Storage = stackMemoryArenaStorage;
    memoryArena.Level = stackMemoryArenaStorage->Level;

    StackMemoryArena result = {};
    result.Arena = memoryArena;
    result.StartOffsetInBytes = SystemGetMemoryArenaAllocatedBytes(memoryArena);
    result.StartExtraOffsetInBytes = extraStorageAllocatedBytes;

    return result;
}

StackMemoryArena::~StackMemoryArena()
{
    auto storage = Arena.Storage;

    if (storage->ExtraStorage.Storage != nullptr)
    {
        auto extraBytesToPop = SystemGetMemoryArenaAllocatedBytes(storage->ExtraStorage) - StartExtraOffsetInBytes;

        if (extraBytesToPop && storage->MinAllocatedLevel >= Arena.Level)
        {
            SystemPopMemory(storage->ExtraStorage, extraBytesToPop);
            storage->MinAllocatedLevel = 255;
        } 
    }

    storage->Level--;

    auto bytesToPop = SystemGetMemoryArenaAllocatedBytes(Arena) - StartOffsetInBytes;

    if (bytesToPop > 0)
    {
        SystemPopMemory(Arena, bytesToPop);
    }
}

struct PageSizeIndexes
{
    size_t StartIndex;
    size_t EndIndex;
};

PageSizeIndexes ComputePageSizeInfoIndexes(MemoryArenaStorage* storage, void* pointer, size_t sizeInBytes)
{
    PageSizeIndexes result = {};
    result.StartIndex = ((uint8_t*)pointer - (uint8_t*)storage) / systemPageSizeInBytes;
    result.EndIndex = (size_t)SystemRoundUp((float)((uint8_t*)pointer - (uint8_t*)storage + sizeInBytes) / systemPageSizeInBytes);
    return result;
}

PageSizeIndexes ComputePageSizeLocalOffsets(MemoryArenaStorage* storage, size_t index, void* pointer, size_t sizeInBytes)
{
    // TODO: Take into account the index
    PageSizeIndexes result = {};
    result.StartIndex = ((uint8_t*)pointer - (uint8_t*)storage) % systemPageSizeInBytes;
    result.EndIndex = ((uint8_t*)pointer - (uint8_t*)storage + sizeInBytes) % systemPageSizeInBytes;
    return result;
}

void SystemCommitMemory(MemoryArena memoryArena, void* pointer, size_t sizeInBytes)
{
    auto storage = memoryArena.Storage;
    auto pageSizeIndexes = ComputePageSizeInfoIndexes(storage, pointer, sizeInBytes);
    auto needToCommit = false;

    for (size_t i = pageSizeIndexes.StartIndex; i < pageSizeIndexes.EndIndex; i++)
    {
        auto pageSizeOffsets = ComputePageSizeLocalOffsets(storage, i, pointer, sizeInBytes);
        auto pageInfos = &storage->PagesInfos[i];

        //printf("Checking page: %llu (size: %llu) (start: %llu, end: %llu, pageSize: %llu, allocated: %llu)\n", i, sizeInBytes, pageSizeIndexes.StartIndex, pageSizeIndexes.EndIndex, systemPageSizeInBytes, SystemGetMemoryArenaAllocatedBytes(memoryArena));
  
            
        // TODO: Atomic (with exchange?)
        pageInfos->MinCommittedOffset = pageSizeOffsets.StartIndex < pageInfos->MinCommittedOffset ? pageSizeOffsets.StartIndex : pageInfos->MinCommittedOffset;
        pageInfos->MaxCommittedOffset = pageSizeOffsets.EndIndex > pageInfos->MaxCommittedOffset ? pageSizeOffsets.EndIndex : pageInfos->MaxCommittedOffset;

        printf("Commit page :%llu start: %u end: %u\n", i, pageSizeOffsets.StartIndex, pageSizeOffsets.EndIndex);
    
        if (!pageInfos->IsCommitted)
        {
            printf("Need to commit page %llu\n", i);
            needToCommit = true;
        }
    }

    if (!needToCommit)
    {
        return;
    }

    auto expected = false;

    while (!__atomic_compare_exchange_n(&storage->IsCommitOperationInProgres, &expected, true, true, __ATOMIC_ACQUIRE, __ATOMIC_ACQUIRE))
    {
        expected = false;
        SystemYieldThread();
    }

    pageSizeIndexes = ComputePageSizeInfoIndexes(storage, pointer, sizeInBytes);

    for (size_t i = pageSizeIndexes.StartIndex; i < pageSizeIndexes.EndIndex; i++)
    {
        // TODO: Try to group the commit
        if (!storage->PagesInfos[i].IsCommitted)
        {
            printf("Committing page %llu\n", i);
            SystemPlatformCommitMemory((uint8_t*)storage + i * systemPageSizeInBytes, systemPageSizeInBytes);
            storage->PagesInfos[i].IsCommitted = true;
            //storage->PagesInfos[i].MinCommittedOffset = systemPageSizeInBytes;
            //storage->PagesInfos[i].MaxCommittedOffset = 0;
        }
    }

    __atomic_store_n(&storage->IsCommitOperationInProgres, false, __ATOMIC_RELEASE);
}

void SystemDecommitMemory(MemoryArena memoryArena, void* pointer, size_t sizeInBytes)
{
    auto storage = memoryArena.Storage;
    auto pageSizeIndexes = ComputePageSizeInfoIndexes(storage, pointer, sizeInBytes);
    auto needToDecommit = false;

    for (size_t i = pageSizeIndexes.StartIndex; i < pageSizeIndexes.EndIndex; i++)
    {
        auto pageSizeOffsets = ComputePageSizeLocalOffsets(storage, i, pointer, sizeInBytes);
        auto pageInfos = &storage->PagesInfos[i];
        // TODO: Need to check start and end index to be sure if we can decommit!
        // TODO: Atomic

        //pageInfos->MinCommittedOffset = pageSizeOffsets.StartIndex >= pageInfos->MinCommittedOffset ? pageSizeOffsets.EndIndex : pageInfos->MinCommittedOffset;
        //pageInfos->MaxCommittedOffset = pageSizeOffsets.EndIndex <= pageInfos->MaxCommittedOffset ? pageSizeOffsets.StartIndex : pageInfos->MaxCommittedOffset;

        printf("Checking page: %llu (size: %llu) (min: %u, max: %u, start offset: %llu, end offset: %llu)\n", i, sizeInBytes, pageInfos->MinCommittedOffset, pageInfos->MaxCommittedOffset
               , pageSizeOffsets.StartIndex, 
               pageSizeOffsets.EndIndex);

        if (pageInfos->IsCommitted && (int32_t)(pageInfos->MaxCommittedOffset - pageInfos->MinCommittedOffset) <= 0)
        {
            printf("Need to decommit page %llu\n", i);
            needToDecommit = true;
            break;
        }
    }

    // Maybe it is possible by storing in each page entry in the table the min and max pointer
    // We update the boundaries and if no more we can decommit safely
}

// TODO: Use span everywhere?
void* SystemPushMemory(MemoryArena memoryArena, size_t sizeInBytes)
{
    // TODO: Don't use atomics if the memory arena equals stack memory arena

    // TODO: Align sizeInBytes
    auto workingMemoryArena = GetStackWorkingMemoryArena(memoryArena);
    auto storage = workingMemoryArena.Storage;
    auto allocatedSize = SystemGetMemoryArenaAllocatedBytes(memoryArena);

    if (allocatedSize + sizeInBytes > storage->SizeInBytes)
    {
        SystemLogErrorMessage(LogMessageCategory_Memory, "Memory Arena size limit!!! (TODO: Message)");
        return nullptr;
    }

    auto pointer = __atomic_fetch_add(&storage->CurrentPointer, sizeInBytes, __ATOMIC_SEQ_CST);
    SystemCommitMemory(workingMemoryArena, pointer, sizeInBytes);

    if (pointer + sizeInBytes >= storage->CommitedMemoryPointer)
    {
        auto expected = false;

        while (!__atomic_compare_exchange_n(&storage->IsCommitOperationInProgres, &expected, true, true, __ATOMIC_ACQUIRE, __ATOMIC_ACQUIRE))
        {
            expected = false;
            SystemYieldThread();
        }

        if (pointer + sizeInBytes >= storage->CommitedMemoryPointer)
        {
            auto sizeResized = ResizeToPageSizeMultiple(sizeInBytes, systemPageSizeInBytes);

            //SystemPlatformCommitMemory(storage->CommitedMemoryPointer, sizeResized);
            storage->CommitedMemoryPointer += sizeResized;
        }

        __atomic_store_n(&storage->IsCommitOperationInProgres, false, __ATOMIC_RELEASE);
    }

    return pointer;
}

void SystemPopMemory(MemoryArena memoryArena, size_t sizeInBytes)
{
    auto storage = memoryArena.Storage;
    auto allocatedSize = SystemGetMemoryArenaAllocatedBytes(memoryArena);

    if (sizeInBytes > allocatedSize)
    {
        SystemLogErrorMessage(LogMessageCategory_Memory, "Cannot pop memory arena with: %u (Allocated size is: %u)", sizeInBytes, allocatedSize);
        return;
    }

    auto currentPointer = __atomic_fetch_add(&storage->CurrentPointer, -sizeInBytes, __ATOMIC_SEQ_CST);
    SystemDecommitMemory(memoryArena, currentPointer - sizeInBytes, sizeInBytes);

    // TODO: Decommit, concurrent dictionary add crash because we don't reset the flags.

    auto availableCommitedSizeInBytes = (int32_t)(storage->CommitedMemoryPointer - (currentPointer - sizeInBytes));
    auto fullPagesToDecommit = availableCommitedSizeInBytes / (int32_t)systemPageSizeInBytes;

    if (fullPagesToDecommit > 1)
    {
        auto expected = false;

        while (!__atomic_compare_exchange_n(&storage->IsCommitOperationInProgres, &expected, true, true, __ATOMIC_ACQUIRE, __ATOMIC_ACQUIRE))
        {
            expected = false;
            SystemYieldThread();
        }

        __atomic_load(&storage->CurrentPointer, &currentPointer, __ATOMIC_ACQUIRE);

        availableCommitedSizeInBytes = (int32_t)(storage->CommitedMemoryPointer - currentPointer);
        fullPagesToDecommit = availableCommitedSizeInBytes / (int32_t)systemPageSizeInBytes;

        if (fullPagesToDecommit > 1)
        {
            auto pageSizesToDecommit = fullPagesToDecommit * systemPageSizeInBytes;

            storage->CommitedMemoryPointer -= pageSizesToDecommit;
            SystemPlatformDecommitMemory(storage->CommitedMemoryPointer, pageSizesToDecommit);
        } 

        __atomic_store_n(&storage->IsCommitOperationInProgres, false, __ATOMIC_RELEASE);
    }
}

void* SystemPushMemoryZero(MemoryArena memoryArena, size_t sizeInBytes)
{
    auto result = SystemPushMemory(memoryArena, sizeInBytes);
    SystemPlatformClearMemory(result, sizeInBytes);

    return result;
}

template<typename T>
Span<T> SystemPushArray(MemoryArena memoryArena, size_t count)
{
    auto memory = SystemPushMemory(memoryArena, sizeof(T) * count);
    return Span<T>((T*)memory, count);
}

template<typename T>
Span<T> SystemPushArrayZero(MemoryArena memoryArena, size_t count)
{
    auto memory = SystemPushMemoryZero(memoryArena, sizeof(T) * count);
    return Span<T>((T*)memory, count);
}

template<>
Span<char> SystemPushArrayZero<char>(MemoryArena memoryArena, size_t count)
{
    auto memory = SystemPushMemoryZero(memoryArena, sizeof(char) * (count + 1));
    return Span<char>((char*)memory, count);
}

template<>
Span<wchar_t> SystemPushArrayZero<wchar_t>(MemoryArena memoryArena, size_t count)
{
    auto memory = SystemPushMemoryZero(memoryArena, sizeof(wchar_t) * (count + 1));
    return Span<wchar_t>((wchar_t*)memory, count);
}

template<typename T>
T* SystemPushStruct(MemoryArena memoryArena)
{
    return (T*)SystemPushMemory(memoryArena, sizeof(T));
}

template<typename T>
T* SystemPushStructZero(MemoryArena memoryArena)
{
    return (T*)SystemPushMemoryZero(memoryArena, sizeof(T));
}

template<typename T>
void SystemCopyBuffer(Span<T> destination, ReadOnlySpan<T> source)
{
    // TODO: Add checks
    SystemPlatformCopyMemory(destination.Pointer, source.Pointer, source.Length * sizeof(T));
}

template<typename T>
Span<T> SystemConcatBuffers(MemoryArena memoryArena, ReadOnlySpan<T> buffer1, ReadOnlySpan<T> buffer2)
{
    auto result = SystemPushArray<T>(memoryArena, buffer1.Length + buffer2.Length);

    SystemCopyBuffer(result, buffer1);
    SystemCopyBuffer(result.Slice(buffer1.Length), buffer2);

    return result;
}

template<>
Span<char> SystemConcatBuffers(MemoryArena memoryArena, ReadOnlySpan<char> buffer1, ReadOnlySpan<char> buffer2)
{
    auto result = SystemPushArrayZero<char>(memoryArena, buffer1.Length + buffer2.Length);

    SystemCopyBuffer(result, buffer1);
    SystemCopyBuffer(result.Slice(buffer1.Length), buffer2);

    return result;
}

template<>
Span<wchar_t> SystemConcatBuffers(MemoryArena memoryArena, ReadOnlySpan<wchar_t> buffer1, ReadOnlySpan<wchar_t> buffer2)
{
    auto result = SystemPushArrayZero<wchar_t>(memoryArena, buffer1.Length + buffer2.Length);

    SystemCopyBuffer(result, buffer1);
    SystemCopyBuffer(result.Slice(buffer1.Length), buffer2);

    return result;
}
