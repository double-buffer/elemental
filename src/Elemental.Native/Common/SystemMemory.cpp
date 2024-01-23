#include "SystemMemory.h"
#include "SystemFunctions.h"
#include "SystemLogging.h"
#include "SystemPlatformFunctions.h"

#define MEMORYARENA_DEFAULT_SIZE 64 * 1024 * 1024

struct MemoryArenaPageInfo
{
    uint16_t MinCommittedOffset;
    uint16_t MaxCommittedOffset;
};

struct MemoryArenaPageCommitInfo 
{
    uint32_t CommittedStates;
};

struct MemoryArenaStorage
{
    uint8_t* CurrentPointer;
    size_t SizeInBytes;
    size_t HeaderSizeInBytes;
    bool IsCommitOperationInProgres;
    size_t CommittedPagesCount;
    MemoryArenaPageInfo* PagesInfos;
    MemoryArenaPageCommitInfo* PagesCommitInfos;

    // Stack fields
    MemoryArena ExtraStorage;
    uint8_t Level;
    uint8_t MinAllocatedLevel;
};

struct PageSizeIndexes
{
    size_t StartIndex;
    size_t EndIndex;
};

thread_local MemoryArenaStorage* stackMemoryArenaStorage = nullptr;
size_t systemPageSizeInBytes = 0;

PageSizeIndexes ComputePageSizeInfoIndexes(MemoryArenaStorage* storage, void* pointer, size_t sizeInBytes)
{
    auto offset = (uint8_t*)pointer - (uint8_t*)storage;

    PageSizeIndexes result = {};
    result.StartIndex = offset / systemPageSizeInBytes;
    result.EndIndex = (size_t)SystemRoundUp((float)(offset + sizeInBytes) / systemPageSizeInBytes);
    return result;
}

PageSizeIndexes ComputePageSizeLocalOffsets(MemoryArenaStorage* storage, size_t index, void* pointer, size_t sizeInBytes)
{
    auto absoluteStart = (uint8_t*)pointer;
    auto absoluteEnd = (uint8_t*)pointer + sizeInBytes;

    auto pageStart = (uint8_t*)storage + index * systemPageSizeInBytes;
    auto pageEnd = pageStart + systemPageSizeInBytes;

    PageSizeIndexes result = {};
    result.StartIndex = absoluteStart > pageStart ? absoluteStart - pageStart : 0;
    result.EndIndex = absoluteEnd < pageEnd ? absoluteEnd - pageStart : systemPageSizeInBytes - 1;

    return result;
}

size_t ResizeToPageSizeMultiple(size_t sizeInBytes, size_t pageSizeInBytes)
{
    return (sizeInBytes + pageSizeInBytes - 1) & ~(pageSizeInBytes - 1);
}

void SetPageCommitted(MemoryArenaStorage* storage, uint32_t pageIndex) 
{
    auto arrayIndex = pageIndex / 32;
    auto bitIndex = pageIndex % 32;
    storage->PagesCommitInfos[arrayIndex].CommittedStates |= (1U << bitIndex);
}

void ClearPageCommitted(MemoryArenaStorage* storage, uint32_t pageIndex)
{
    auto arrayIndex = pageIndex / 32;
    auto bitIndex = pageIndex % 32;
    storage->PagesCommitInfos[arrayIndex].CommittedStates &= ~(1U << bitIndex);
}

bool IsPageCommitted(MemoryArenaStorage* storage, uint32_t pageIndex) 
{
    auto arrayIndex = pageIndex / 32;
    auto bitIndex = pageIndex % 32;
    return (storage->PagesCommitInfos[arrayIndex].CommittedStates & (1U << bitIndex)) != 0;
}

#include <stdio.h>
MemoryArenaStorage* AllocateMemoryArenaStorage(size_t sizeInBytes)
{
    if (systemPageSizeInBytes == 0)
    {
        systemPageSizeInBytes = SystemPlatformGetPageSize();
    }

    auto pageInfosCount = SystemRoundUp((float)sizeInBytes / (float)systemPageSizeInBytes);
    auto headerSizeInBytes = sizeof(MemoryArenaStorage) + pageInfosCount * sizeof(MemoryArenaPageInfo) + SystemRoundUp((float)pageInfosCount / 32) * sizeof(MemoryArenaPageCommitInfo);

    auto sizeResized = ResizeToPageSizeMultiple(headerSizeInBytes + sizeInBytes, systemPageSizeInBytes);
    auto storage = (MemoryArenaStorage*)SystemPlatformReserveMemory(sizeResized);

    auto headerResized = ResizeToPageSizeMultiple(headerSizeInBytes, systemPageSizeInBytes);
    SystemPlatformCommitMemory(storage, headerResized);

    storage->CurrentPointer = (uint8_t*)storage + headerSizeInBytes;
    storage->SizeInBytes = sizeInBytes;
    storage->HeaderSizeInBytes = headerSizeInBytes;
    storage->IsCommitOperationInProgres = false;
    storage->CommittedPagesCount = 0;
    storage->PagesInfos = (MemoryArenaPageInfo*)((uint8_t*)storage + sizeof(MemoryArenaStorage));
    storage->PagesCommitInfos = (MemoryArenaPageCommitInfo*)((uint8_t*)storage + sizeof(MemoryArenaStorage) + pageInfosCount * sizeof(MemoryArenaPageInfo));
    storage->ExtraStorage = {};
    storage->Level = 0;
    storage->MinAllocatedLevel = 255;

    printf("PageInfo Count: %llu, HeaderSize: %llu, SystemPageSize: %llu\n", pageInfosCount, storage->HeaderSizeInBytes, systemPageSizeInBytes);

    for (size_t i = 0; i < (size_t)pageInfosCount; i++)
    {
        if (headerResized / systemPageSizeInBytes > i)
        {
            auto offsets = ComputePageSizeLocalOffsets(storage, i, storage, headerSizeInBytes);

            SetPageCommitted(storage, i);
            storage->CommittedPagesCount++;
            storage->PagesInfos[i].MinCommittedOffset = offsets.StartIndex;
            storage->PagesInfos[i].MaxCommittedOffset = offsets.EndIndex;
        }
        else 
        {
            ClearPageCommitted(storage, i);
            storage->PagesInfos[i].MinCommittedOffset = systemPageSizeInBytes;
            storage->PagesInfos[i].MaxCommittedOffset = 0;
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

size_t GetMemoryArenaAllocatedBytes(MemoryArena memoryArena)
{
    return memoryArena.Storage->CurrentPointer - (uint8_t*)memoryArena.Storage - memoryArena.Storage->HeaderSizeInBytes;
}

MemoryArena SystemAllocateMemoryArena()
{
    return SystemAllocateMemoryArena(MEMORYARENA_DEFAULT_SIZE);
}

MemoryArena SystemAllocateMemoryArena(size_t sizeInBytes)
{
    MemoryArena result = {};
    result.Storage = AllocateMemoryArenaStorage(sizeInBytes);
    result.Level = 0;
    return result;
}

void SystemFreeMemoryArena(MemoryArena memoryArena)
{
    SystemPlatformFreeMemory(memoryArena.Storage, memoryArena.Storage->HeaderSizeInBytes + memoryArena.Storage->SizeInBytes);
}

void SystemClearMemoryArena(MemoryArena memoryArena)
{
    SystemPopMemory(memoryArena, GetMemoryArenaAllocatedBytes(memoryArena));
}

AllocationInfos SystemGetAllocationInfos()
{
    auto allocationInfos = SystemPlatformGetAllocationInfos();

    AllocationInfos result = {};
    result.CommittedBytes = allocationInfos.CommittedBytes;
    result.ReservedBytes = allocationInfos.ReservedBytes;

    return result;
}

MemoryArenaAllocationInfos SystemGetMemoryArenaAllocationInfos(MemoryArena memoryArena)
{
    MemoryArenaAllocationInfos result = {};
    result.AllocatedBytes = GetMemoryArenaAllocatedBytes(memoryArena);
    result.CommittedBytes = memoryArena.Storage->CommittedPagesCount * systemPageSizeInBytes;
    result.MaximumSizeInBytes = memoryArena.Storage->SizeInBytes;

    return result;
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
        extraStorageAllocatedBytes = GetMemoryArenaAllocatedBytes(stackMemoryArenaStorage->ExtraStorage);
    }

    MemoryArena memoryArena = {};
    memoryArena.Storage = stackMemoryArenaStorage;
    memoryArena.Level = stackMemoryArenaStorage->Level;

    StackMemoryArena result = {};
    result.Arena = memoryArena;
    result.StartOffsetInBytes = GetMemoryArenaAllocatedBytes(memoryArena);
    result.StartExtraOffsetInBytes = extraStorageAllocatedBytes;

    return result;
}

StackMemoryArena::~StackMemoryArena()
{
    auto storage = Arena.Storage;

    if (storage->ExtraStorage.Storage != nullptr)
    {
        auto extraBytesToPop = GetMemoryArenaAllocatedBytes(storage->ExtraStorage) - StartExtraOffsetInBytes;

        if (extraBytesToPop && storage->MinAllocatedLevel >= Arena.Level)
        {
            SystemPopMemory(storage->ExtraStorage, extraBytesToPop);
            storage->MinAllocatedLevel = 255;
        } 
    }

    storage->Level--;

    auto bytesToPop = GetMemoryArenaAllocatedBytes(Arena) - StartOffsetInBytes;

    if (bytesToPop > 0)
    {
        SystemPopMemory(Arena, bytesToPop);
    }
}

bool showDebug = true;

void DebugPageTable(MemoryArenaStorage* storage, size_t count)
{
    for (size_t i = 0; i < count; i++)
    {
        auto pageSizeInfos = storage->PagesInfos[i];
        printf("Page %llu: IsPageCommitted=%u, Min=%u, Max=%u\n", i, IsPageCommitted(storage, i), pageSizeInfos.MinCommittedOffset, pageSizeInfos.MaxCommittedOffset);
    }

    printf("================================\n");
}

void SystemCommitMemory(MemoryArena memoryArena, void* pointer, size_t sizeInBytes)
{
    auto storage = memoryArena.Storage;
    auto pageSizeIndexes = ComputePageSizeInfoIndexes(storage, pointer, sizeInBytes);
    auto needToCommit = false;

    if (showDebug)
    {
        printf("================================\n");
        printf("Begin Commit: %llu\n", sizeInBytes);
        DebugPageTable(storage, 10);
    }

    for (size_t i = pageSizeIndexes.StartIndex; i < pageSizeIndexes.EndIndex; i++)
    {
        auto pageSizeOffsets = ComputePageSizeLocalOffsets(storage, i, pointer, sizeInBytes);
        auto pageInfos = &storage->PagesInfos[i];

        if (memoryArena.Storage == stackMemoryArenaStorage)
        {
            pageInfos->MinCommittedOffset = pageSizeOffsets.StartIndex < pageInfos->MinCommittedOffset ? pageSizeOffsets.StartIndex : pageInfos->MinCommittedOffset;
            pageInfos->MaxCommittedOffset = pageSizeOffsets.EndIndex > pageInfos->MaxCommittedOffset ? pageSizeOffsets.EndIndex : pageInfos->MaxCommittedOffset;
        }
        else
        {
            SystemAtomicReplace(pageInfos->MinCommittedOffset, pageInfos->MinCommittedOffset, pageSizeOffsets.StartIndex < pageInfos->MinCommittedOffset ? pageSizeOffsets.StartIndex : pageInfos->MinCommittedOffset);
            SystemAtomicReplace(pageInfos->MaxCommittedOffset, pageInfos->MaxCommittedOffset, pageSizeOffsets.EndIndex > pageInfos->MaxCommittedOffset ? pageSizeOffsets.EndIndex : pageInfos->MaxCommittedOffset);
        }

        if (!IsPageCommitted(storage, i))
        {
            if (showDebug)
            {
                printf("Need to commit page %llu\n", i);
            }
            needToCommit = true;
        }
    }

    if (!needToCommit)
    {
        return;
    }

    if (memoryArena.Storage != stackMemoryArenaStorage)
    {
        SystemAtomicReplaceWithValue(storage->IsCommitOperationInProgres, false, true);
    }

    pageSizeIndexes = ComputePageSizeInfoIndexes(storage, pointer, sizeInBytes);

    for (size_t i = pageSizeIndexes.StartIndex; i < pageSizeIndexes.EndIndex; i++)
    {
        // TODO: Try to group the commit
        if (!IsPageCommitted(storage, i))
        {
            if (showDebug)
            {
                printf("Committing page %llu\n", i);
            }
            SystemPlatformCommitMemory((uint8_t*)storage + i * systemPageSizeInBytes, systemPageSizeInBytes);
            SetPageCommitted(storage, i);
            storage->CommittedPagesCount++;
        }
    }

    if (memoryArena.Storage != stackMemoryArenaStorage)
    {
        SystemAtomicStore(storage->IsCommitOperationInProgres, false);
    }

    if (showDebug)
    {
        DebugPageTable(storage, 10);
    }
}

void SystemDecommitMemory(MemoryArena memoryArena, void* pointer, size_t sizeInBytes)
{
    auto storage = memoryArena.Storage;
    auto pageSizeIndexes = ComputePageSizeInfoIndexes(storage, pointer, sizeInBytes);
    auto needToDecommit = false;

    if (showDebug)
    {
        printf("================================\n");
        printf("Begin Decommit: %llu\n", sizeInBytes);
        DebugPageTable(storage, 10);
    }

    for (size_t i = pageSizeIndexes.StartIndex; i < pageSizeIndexes.EndIndex; i++)
    {
        auto pageSizeOffsets = ComputePageSizeLocalOffsets(storage, i, pointer, sizeInBytes);
        auto pageInfos = &storage->PagesInfos[i];

        if (memoryArena.Storage == stackMemoryArenaStorage)
        {
            pageInfos->MinCommittedOffset = pageSizeOffsets.StartIndex == pageInfos->MinCommittedOffset ? pageSizeOffsets.EndIndex : pageInfos->MinCommittedOffset;
            pageInfos->MaxCommittedOffset = pageSizeOffsets.EndIndex == pageInfos->MaxCommittedOffset ? pageSizeOffsets.StartIndex : pageInfos->MaxCommittedOffset;
        }
        else
        {
            SystemAtomicReplace(pageInfos->MinCommittedOffset, pageInfos->MinCommittedOffset, pageSizeOffsets.StartIndex == pageInfos->MinCommittedOffset ? pageSizeOffsets.EndIndex : pageInfos->MinCommittedOffset);
            SystemAtomicReplace(pageInfos->MaxCommittedOffset, pageInfos->MaxCommittedOffset, pageSizeOffsets.EndIndex == pageInfos->MaxCommittedOffset ? pageSizeOffsets.StartIndex : pageInfos->MaxCommittedOffset);
        }

        if (IsPageCommitted(storage, i) && (int32_t)(pageInfos->MaxCommittedOffset - pageInfos->MinCommittedOffset) <= 0)
        {
            if (showDebug)
            {
                printf("Need to decommit page %llu\n", i);
            }
            needToDecommit = true;
        }
    }

    if (showDebug)
    {
        DebugPageTable(storage, 10);
    }

    if (!needToDecommit)
    {
        return;
    }

    if (memoryArena.Storage != stackMemoryArenaStorage)
    {
        SystemAtomicReplaceWithValue(storage->IsCommitOperationInProgres, false, true);
    }

    for (size_t i = pageSizeIndexes.StartIndex; i < pageSizeIndexes.EndIndex; i++)
    {
        auto pageInfos = &storage->PagesInfos[i];

        if (IsPageCommitted(storage, i) && (int32_t)(pageInfos->MaxCommittedOffset - pageInfos->MinCommittedOffset) <= 0)
        {
            if (showDebug)
            {
                printf("Decommit page %llu\n", i);
            }
            SystemPlatformDecommitMemory((uint8_t*)storage + i * systemPageSizeInBytes, systemPageSizeInBytes);
            ClearPageCommitted(storage, i);
            storage->CommittedPagesCount--;
        }
    }

    if (memoryArena.Storage != stackMemoryArenaStorage)
    {
        SystemAtomicStore(storage->IsCommitOperationInProgres, false);
    }
}

// TODO: Use span everywhere?
void* SystemPushMemory(MemoryArena memoryArena, size_t sizeInBytes)
{
    // TODO: Align sizeInBytes
    auto workingMemoryArena = GetStackWorkingMemoryArena(memoryArena);
    auto storage = workingMemoryArena.Storage;
    auto allocatedSize = GetMemoryArenaAllocatedBytes(memoryArena);

    if (allocatedSize + sizeInBytes > storage->SizeInBytes)
    {
        SystemLogErrorMessage(LogMessageCategory_Memory, "Cannot push to memory arena with: %u (Allocated size is: %u, Max size is: %u)", sizeInBytes, allocatedSize, storage->SizeInBytes);
        return nullptr;
    }

    uint8_t* pointer;

    if (memoryArena.Storage == stackMemoryArenaStorage)
    {
        pointer = storage->CurrentPointer;
        storage->CurrentPointer += sizeInBytes;
    }
    else
    {
        pointer = SystemAtomicAdd(storage->CurrentPointer, sizeInBytes);
    }

    SystemCommitMemory(workingMemoryArena, pointer, sizeInBytes);

    return pointer;
}

void SystemPopMemory(MemoryArena memoryArena, size_t sizeInBytes)
{
    auto storage = memoryArena.Storage;
    auto allocatedSize = GetMemoryArenaAllocatedBytes(memoryArena);

    if (sizeInBytes > allocatedSize)
    {
        SystemLogErrorMessage(LogMessageCategory_Memory, "Cannot pop memory arena with: %u (Allocated size is: %u)", sizeInBytes, allocatedSize);
        return;
    }

    uint8_t* pointer;

    if (memoryArena.Storage == stackMemoryArenaStorage)
    {
        pointer = storage->CurrentPointer;
        storage->CurrentPointer -= sizeInBytes;
    }
    else
    {
        pointer = SystemAtomicSubstract(storage->CurrentPointer, sizeInBytes);
    }

    SystemDecommitMemory(memoryArena, pointer - sizeInBytes, sizeInBytes);
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
