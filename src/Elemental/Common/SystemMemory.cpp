#include "SystemMemory.h"
#include "SystemFunctions.h"
#include "SystemPlatformFunctions.h"

#ifdef ElemAPI
#include "SystemLogging.h"
#else
#define SystemLogErrorMessage(category, format, ...)
#endif

#define MEMORYARENA_DEFAULT_SIZE 64 * 1024 * 1024
#define MEMORYARENA_DEFAULT_ALIGNMENT 8

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

    MemoryArena StackExtraStorage;
    uint8_t StackLevel;
    uint8_t StackMinAllocatedLevel;
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
    storage->StackExtraStorage = {};
    storage->StackLevel = 0;
    storage->StackMinAllocatedLevel = 255;

    auto headerPageCount = (size_t)SystemRoundUp((float)headerResized / systemPageSizeInBytes);

    for (size_t i = 0; i < (size_t)pageInfosCount; i++)
    {
        if (headerPageCount > i)
        {
            auto offsets = ComputePageSizeLocalOffsets(storage, i, storage, headerSizeInBytes);

            SetPageCommitted(storage, (uint32_t)i);
            storage->CommittedPagesCount++;
            storage->PagesInfos[i].MinCommittedOffset = offsets.StartIndex;
            storage->PagesInfos[i].MaxCommittedOffset = offsets.EndIndex;
        }
        else 
        {
            ClearPageCommitted(storage, (uint32_t)i);
            storage->PagesInfos[i].MinCommittedOffset = systemPageSizeInBytes - 1;
            storage->PagesInfos[i].MaxCommittedOffset = 0;
        }
    }
    
    return storage;
}

MemoryArena GetStackWorkingMemoryArena(MemoryArena memoryArena)
{
    MemoryArena workingMemoryArena = memoryArena;

    if (memoryArena.Level != memoryArena.Storage->StackLevel)
    {
        if (memoryArena.Storage->StackExtraStorage.Storage == nullptr)
        {
            auto extraHandle = AllocateMemoryArenaStorage(MEMORYARENA_DEFAULT_SIZE);
            memoryArena.Storage->StackExtraStorage = { extraHandle, 0 };
        }

        workingMemoryArena = memoryArena.Storage->StackExtraStorage;
        memoryArena.Storage->StackMinAllocatedLevel = SystemMin(memoryArena.Storage->StackMinAllocatedLevel, memoryArena.Level);
    }

    return workingMemoryArena;
}

size_t GetMemoryArenaAllocatedBytes(MemoryArena memoryArena)
{
    return memoryArena.Storage->CurrentPointer - (uint8_t*)memoryArena.Storage - memoryArena.Storage->HeaderSizeInBytes;
}

AllocationInfos SystemGetAllocationInfos()
{
    auto allocationInfos = SystemPlatformGetAllocationInfos();

    AllocationInfos result = {};
    result.CommittedBytes = allocationInfos.CommittedBytes;
    result.ReservedBytes = allocationInfos.ReservedBytes;

    return result;
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

    stackMemoryArenaStorage->StackLevel++;
    auto extraStorageAllocatedBytes = 0llu;

    if (stackMemoryArenaStorage->StackExtraStorage.Storage != nullptr)
    {
        extraStorageAllocatedBytes = GetMemoryArenaAllocatedBytes(stackMemoryArenaStorage->StackExtraStorage);
    }

    MemoryArena memoryArena = {};
    memoryArena.Storage = stackMemoryArenaStorage;
    memoryArena.Level = stackMemoryArenaStorage->StackLevel;

    StackMemoryArena result = {};
    result.Arena = memoryArena;
    result.StartOffsetInBytes = GetMemoryArenaAllocatedBytes(memoryArena);
    result.StartExtraOffsetInBytes = extraStorageAllocatedBytes;

    return result;
}

StackMemoryArena::~StackMemoryArena()
{
    auto storage = Arena.Storage;

    if (storage->StackExtraStorage.Storage != nullptr)
    {
        auto extraBytesToPop = GetMemoryArenaAllocatedBytes(storage->StackExtraStorage) - StartExtraOffsetInBytes;

        if (extraBytesToPop && storage->StackMinAllocatedLevel >= Arena.Level)
        {
            SystemPopMemory(storage->StackExtraStorage, extraBytesToPop);
            storage->StackMinAllocatedLevel = 255;
        } 
    }

    storage->StackLevel--;

    auto bytesToPop = GetMemoryArenaAllocatedBytes(Arena) - StartOffsetInBytes;

    if (bytesToPop > 0)
    {
        SystemPopMemory(Arena, bytesToPop);
    }
}

template<typename T>
void SystemCommitMemory(MemoryArena memoryArena, ReadOnlySpan<T> buffer, bool clearMemory)
{
    SystemCommitMemory(memoryArena, (uint8_t*)buffer.Pointer, sizeof(T) * buffer.Length, true);
}

void SystemCommitMemory(MemoryArena memoryArena, void* pointer, size_t sizeInBytes, bool clearMemory)
{
    auto storage = memoryArena.Storage;
    auto offset = (uint8_t*)pointer - ((uint8_t*)storage + storage->HeaderSizeInBytes);

    if (offset < 0 || offset + sizeInBytes > storage->SizeInBytes)
    {
        return;
    }

    auto pageSizeIndexes = ComputePageSizeInfoIndexes(storage, pointer, sizeInBytes);
    auto needToCommit = false;

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

        if (!IsPageCommitted(storage, (uint32_t)i))
        {
            needToCommit = true;
        }
    }

    if (!needToCommit)
    {
        if (memoryArena.Storage == stackMemoryArenaStorage)
        {
            SystemPlatformClearMemory(pointer, sizeInBytes);
        }

        return;
    }

    if (memoryArena.Storage != stackMemoryArenaStorage)
    {
        SystemAtomicReplace(storage->IsCommitOperationInProgres, false, true);
    }

    pageSizeIndexes = ComputePageSizeInfoIndexes(storage, pointer, sizeInBytes);

    for (size_t i = pageSizeIndexes.StartIndex; i < pageSizeIndexes.EndIndex; i++)
    {
        if (!IsPageCommitted(storage, (uint32_t)i))
        {
            SystemPlatformCommitMemory((uint8_t*)storage + i * systemPageSizeInBytes, systemPageSizeInBytes);
            
            if (clearMemory)
            {
                SystemPlatformClearMemory((uint8_t*)storage + i * systemPageSizeInBytes, systemPageSizeInBytes);
            }

            SetPageCommitted(storage, (uint32_t)i);
            storage->CommittedPagesCount++;
        }
    }

    if (memoryArena.Storage != stackMemoryArenaStorage)
    {
        SystemAtomicStore(storage->IsCommitOperationInProgres, false);
    }

    if (memoryArena.Storage == stackMemoryArenaStorage)
    {
        SystemPlatformClearMemory(pointer, sizeInBytes);
    }
}

void SystemDecommitMemory(MemoryArena memoryArena, void* pointer, size_t sizeInBytes)
{
    auto storage = memoryArena.Storage;
    auto offset = (uint8_t*)pointer - ((uint8_t*)storage + storage->HeaderSizeInBytes);

    if (offset < 0 || offset + sizeInBytes > storage->SizeInBytes)
    {
        return;
    }

    auto pageSizeIndexes = ComputePageSizeInfoIndexes(storage, pointer, sizeInBytes);
    auto needToDecommit = false;

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

        if (IsPageCommitted(storage, (uint32_t)i) && (int32_t)(pageInfos->MaxCommittedOffset - pageInfos->MinCommittedOffset) <= 0)
        {
            needToDecommit = true;
        }
    }

    if (!needToDecommit)
    {
        return;
    }

    if (memoryArena.Storage != stackMemoryArenaStorage)
    {
        SystemAtomicReplace(storage->IsCommitOperationInProgres, false, true);
    }
    
    for (size_t i = pageSizeIndexes.StartIndex; i < pageSizeIndexes.EndIndex; i++)
    {
        auto pageInfos = &storage->PagesInfos[i];

        if (IsPageCommitted(storage, (uint32_t)i))
        {
            auto pagePointer = (uint8_t*)storage + i * systemPageSizeInBytes;

            if ((int32_t)(pageInfos->MaxCommittedOffset - pageInfos->MinCommittedOffset) <= 0)
            {
                SystemPlatformDecommitMemory(pagePointer, systemPageSizeInBytes);
                ClearPageCommitted(storage, (uint32_t)i);
                storage->CommittedPagesCount--;
            }
        }
    }

    if (memoryArena.Storage != stackMemoryArenaStorage)
    {
        SystemAtomicStore(storage->IsCommitOperationInProgres, false);
    }
}

void* SystemPushMemory(MemoryArena memoryArena, size_t sizeInBytes, AllocationState state)
{
    sizeInBytes = SystemAlignToPowerOf2(sizeInBytes, MEMORYARENA_DEFAULT_ALIGNMENT);

    auto workingMemoryArena = GetStackWorkingMemoryArena(memoryArena);
    auto storage = workingMemoryArena.Storage;
    auto allocatedSize = GetMemoryArenaAllocatedBytes(memoryArena);

    if (allocatedSize + sizeInBytes > storage->SizeInBytes)
    {
        SystemLogErrorMessage(ElemLogMessageCategory_Memory, "Cannot push to memory arena with: %u (Allocated size is: %u, Max size is: %u)", sizeInBytes, allocatedSize, storage->SizeInBytes);
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

    if (state == AllocationState_Committed)
    {
        SystemCommitMemory(workingMemoryArena, pointer, sizeInBytes);
    }

    return pointer;
}

void SystemPopMemory(MemoryArena memoryArena, size_t sizeInBytes)
{
    auto storage = memoryArena.Storage;
    auto allocatedSize = GetMemoryArenaAllocatedBytes(memoryArena);

    if (sizeInBytes > allocatedSize)
    {
        SystemLogErrorMessage(ElemLogMessageCategory_Memory, "Cannot pop memory arena with: %u (Allocated size is: %u)", sizeInBytes, allocatedSize);
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
        SystemDecommitMemory(memoryArena, pointer - sizeInBytes, sizeInBytes);
    }
}

void* SystemPushMemoryZero(MemoryArena memoryArena, size_t sizeInBytes)
{
    auto result = SystemPushMemory(memoryArena, sizeInBytes);
    SystemPlatformClearMemory(result, sizeInBytes);

    return result;
}

template<typename T>
Span<T> SystemPushArray(MemoryArena memoryArena, size_t count, AllocationState state)
{
    auto memory = SystemPushMemory(memoryArena, sizeof(T) * count, state);
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
    if (destination.Length < source.Length)
    {
        SystemLogErrorMessage(ElemLogMessageCategory_Memory, "Cannot copy buffer, destination length is less than source length.");
        return;
    }

    SystemPlatformCopyMemory(destination.Pointer, source.Pointer, source.Length * sizeof(T));
}

template<typename T>
Span<T> SystemDuplicateBuffer(MemoryArena memoryArena, ReadOnlySpan<T> source)
{
    auto result = SystemPushArray<T>(memoryArena, source.Length);
    SystemCopyBuffer(result, source);
    return result;
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
