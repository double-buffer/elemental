#include "SystemMemory.h"
#include "SystemFunctions.h"
#include "SystemPlatformFunctions.h"

#ifdef ElemAPI
#include "SystemLogging.h"
#else
#define SystemLogErrorMessage(category, format, ...) printf(format __VA_OPT__(,) __VA_ARGS__)
#endif

#define MEMORYARENA_DEFAULT_SIZE 128 * 1024 * 1024
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
const size_t systemPageSizeInBytes = SystemPlatformGetPageSize();

void PopStackMemory(MemoryArena memoryArena, size_t sizeInBytes);
size_t ResizeToPageSizeMultiple(size_t sizeInBytes, size_t pageSizeInBytes);

PageSizeIndexes ComputePageSizeInfoIndexes(MemoryArenaStorage* storage, void* pointer, size_t sizeInBytes)
{
    auto dataStart = (uint8_t*)storage + storage->HeaderSizeInBytes;
    auto offset = (uint8_t*)pointer - dataStart;

    PageSizeIndexes result = {};
    result.StartIndex = offset / systemPageSizeInBytes;
    result.EndIndex = ResizeToPageSizeMultiple(offset + sizeInBytes, systemPageSizeInBytes) / systemPageSizeInBytes;
    return result;
}

PageSizeIndexes ComputePageSizeLocalOffsets(MemoryArenaStorage* storage, size_t index, void* pointer, size_t sizeInBytes)
{
    auto absoluteStart = (uint8_t*)pointer;
    auto absoluteEnd = (uint8_t*)pointer + sizeInBytes;

    auto pageStart = (uint8_t*)storage + storage->HeaderSizeInBytes + index * systemPageSizeInBytes;
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

void LockMemoryArenaCommitOperations(MemoryArenaStorage* storage)
{
    SystemAtomicReplace(storage->IsCommitOperationInProgres, false, true);
}

void UnlockMemoryArenaCommitOperations(MemoryArenaStorage* storage)
{
    SystemAtomicStore(storage->IsCommitOperationInProgres, false);
}

MemoryArenaStorage* AllocateMemoryArenaStorage(size_t sizeInBytes)
{
    auto dataSizeInBytes = ResizeToPageSizeMultiple(sizeInBytes, systemPageSizeInBytes);
    auto pageInfosCount = dataSizeInBytes / systemPageSizeInBytes;
    auto pageCommitInfosCount = (pageInfosCount + 31) / 32;
    auto headerMetadataSizeInBytes = sizeof(MemoryArenaStorage) + pageInfosCount * sizeof(MemoryArenaPageInfo) + pageCommitInfosCount * sizeof(MemoryArenaPageCommitInfo);
    auto headerSizeInBytes = ResizeToPageSizeMultiple(headerMetadataSizeInBytes, systemPageSizeInBytes);
    auto reservedSizeInBytes = headerSizeInBytes + dataSizeInBytes;

    auto storage = (MemoryArenaStorage*)SystemPlatformReserveMemory(reservedSizeInBytes);
    SystemPlatformCommitMemory(storage, headerSizeInBytes);

    storage->CurrentPointer = (uint8_t*)storage + headerSizeInBytes;
    storage->SizeInBytes = sizeInBytes;
    storage->HeaderSizeInBytes = headerSizeInBytes;
    storage->IsCommitOperationInProgres = false;
    storage->CommittedPagesCount = headerSizeInBytes / systemPageSizeInBytes;
    storage->PagesInfos = (MemoryArenaPageInfo*)((uint8_t*)storage + sizeof(MemoryArenaStorage));
    storage->PagesCommitInfos = (MemoryArenaPageCommitInfo*)((uint8_t*)storage + sizeof(MemoryArenaStorage) + pageInfosCount * sizeof(MemoryArenaPageInfo));
    storage->StackExtraStorage = {};
    storage->StackLevel = 0;
    storage->StackMinAllocatedLevel = 255;

    for (size_t i = 0; i < pageInfosCount; i++)
    {
        ClearPageCommitted(storage, (uint32_t)i);
        storage->PagesInfos[i].MinCommittedOffset = systemPageSizeInBytes - 1;
        storage->PagesInfos[i].MaxCommittedOffset = 0;
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

bool IsStackMemoryArena(MemoryArena memoryArena)
{
    if (stackMemoryArenaStorage == nullptr)
    {
        return false;
    }

    return memoryArena.Storage == stackMemoryArenaStorage ||
        (stackMemoryArenaStorage->StackExtraStorage.Storage != nullptr && memoryArena.Storage == stackMemoryArenaStorage->StackExtraStorage.Storage);
}

size_t GetMemoryArenaAllocatedBytes(MemoryArena memoryArena)
{
    uint8_t* currentPointer;

    if (IsStackMemoryArena(memoryArena))
    {
        currentPointer = memoryArena.Storage->CurrentPointer;
    }
    else
    {
        SystemAtomicLoad(memoryArena.Storage->CurrentPointer, currentPointer);
    }

    return currentPointer - (uint8_t*)memoryArena.Storage - memoryArena.Storage->HeaderSizeInBytes;
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
    auto dataSizeInBytes = ResizeToPageSizeMultiple(memoryArena.Storage->SizeInBytes, systemPageSizeInBytes);
    SystemPlatformFreeMemory(memoryArena.Storage, memoryArena.Storage->HeaderSizeInBytes + dataSizeInBytes);
}

void SystemClearMemoryArena(MemoryArena memoryArena)
{
    auto storage = memoryArena.Storage;
    auto allocatedSize = GetMemoryArenaAllocatedBytes(memoryArena);

    if (allocatedSize == 0)
    {
        return;
    }

    auto pointer = storage->CurrentPointer;
    storage->CurrentPointer -= allocatedSize;

    if (memoryArena.Storage != stackMemoryArenaStorage)
    {
        SystemDecommitMemory(memoryArena, pointer - allocatedSize, allocatedSize);
    }
}

MemoryArenaAllocationInfos SystemGetMemoryArenaAllocationInfos(MemoryArena memoryArena)
{
    size_t committedPagesCount;

    if (IsStackMemoryArena(memoryArena))
    {
        committedPagesCount = memoryArena.Storage->CommittedPagesCount;
    }
    else
    {
        SystemAtomicLoad(memoryArena.Storage->CommittedPagesCount, committedPagesCount);
    }

    MemoryArenaAllocationInfos result = {};
    result.AllocatedBytes = GetMemoryArenaAllocatedBytes(memoryArena);
    result.CommittedBytes = committedPagesCount * systemPageSizeInBytes;
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
            PopStackMemory(storage->StackExtraStorage, extraBytesToPop);
            storage->StackMinAllocatedLevel = 255;
        } 
    }

    storage->StackLevel--;

    auto bytesToPop = GetMemoryArenaAllocatedBytes(Arena) - StartOffsetInBytes;

    if (bytesToPop > 0)
    {
        PopStackMemory(Arena, bytesToPop);
    }
}

template<typename T>
void SystemCommitMemory(MemoryArena memoryArena, ReadOnlySpan<T> buffer, bool clearMemory)
{
    SystemCommitMemory(memoryArena, (uint8_t*)buffer.Pointer, sizeof(T) * buffer.Length, clearMemory);
}

void SystemCommitMemory(MemoryArena memoryArena, void* pointer, size_t sizeInBytes, bool clearMemory)
{
    auto storage = memoryArena.Storage;
    auto offset = (uint8_t*)pointer - ((uint8_t*)storage + storage->HeaderSizeInBytes);

    if (offset < 0 || offset + sizeInBytes > storage->SizeInBytes)
    {
        return;
    }

    auto needsSynchronization = !IsStackMemoryArena(memoryArena);

    if (needsSynchronization)
    {
        LockMemoryArenaCommitOperations(storage);
    }

    auto pageSizeIndexes = ComputePageSizeInfoIndexes(storage, pointer, sizeInBytes);
    auto needToCommit = false;

    for (size_t i = pageSizeIndexes.StartIndex; i < pageSizeIndexes.EndIndex; i++)
    {
        auto pageSizeOffsets = ComputePageSizeLocalOffsets(storage, i, pointer, sizeInBytes);
        auto pageInfos = &storage->PagesInfos[i];

        pageInfos->MinCommittedOffset = pageSizeOffsets.StartIndex < pageInfos->MinCommittedOffset ? pageSizeOffsets.StartIndex : pageInfos->MinCommittedOffset;
        pageInfos->MaxCommittedOffset = pageSizeOffsets.EndIndex > pageInfos->MaxCommittedOffset ? pageSizeOffsets.EndIndex : pageInfos->MaxCommittedOffset;

        if (!IsPageCommitted(storage, (uint32_t)i))
        {
            needToCommit = true;
        }
    }

    if (!needToCommit)
    {
        if (needsSynchronization)
        {
            UnlockMemoryArenaCommitOperations(storage);
        }

        if (memoryArena.Storage == stackMemoryArenaStorage)
        {
            SystemPlatformClearMemory(pointer, sizeInBytes);
        }

        return;
    }

    for (size_t i = pageSizeIndexes.StartIndex; i < pageSizeIndexes.EndIndex; i++)
    {
        if (!IsPageCommitted(storage, (uint32_t)i))
        {
            auto pagePointer = (uint8_t*)storage + storage->HeaderSizeInBytes + i * systemPageSizeInBytes;
            SystemPlatformCommitMemory(pagePointer, systemPageSizeInBytes);
            
            if (clearMemory)
            {
                SystemPlatformClearMemory(pagePointer, systemPageSizeInBytes);
            }

            SetPageCommitted(storage, (uint32_t)i);

            if (needsSynchronization)
            {
                SystemAtomicAdd(storage->CommittedPagesCount, 1);
            }
            else
            {
                storage->CommittedPagesCount++;
            }
        }
    }

    if (needsSynchronization)
    {
        UnlockMemoryArenaCommitOperations(storage);
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

    auto needsSynchronization = !IsStackMemoryArena(memoryArena);

    if (needsSynchronization)
    {
        LockMemoryArenaCommitOperations(storage);
    }

    auto pageSizeIndexes = ComputePageSizeInfoIndexes(storage, pointer, sizeInBytes);
    auto needToDecommit = false;

    for (size_t i = pageSizeIndexes.StartIndex; i < pageSizeIndexes.EndIndex; i++)
    {
        auto pageSizeOffsets = ComputePageSizeLocalOffsets(storage, i, pointer, sizeInBytes);
        auto pageInfos = &storage->PagesInfos[i];

        pageInfos->MinCommittedOffset = pageSizeOffsets.StartIndex == pageInfos->MinCommittedOffset ? pageSizeOffsets.EndIndex : pageInfos->MinCommittedOffset;
        pageInfos->MaxCommittedOffset = pageSizeOffsets.EndIndex == pageInfos->MaxCommittedOffset ? pageSizeOffsets.StartIndex : pageInfos->MaxCommittedOffset;

        if (IsPageCommitted(storage, (uint32_t)i) && (int32_t)(pageInfos->MaxCommittedOffset - pageInfos->MinCommittedOffset) <= 0)
        {
            needToDecommit = true;
        }
    }

    if (!needToDecommit)
    {
        if (needsSynchronization)
        {
            UnlockMemoryArenaCommitOperations(storage);
        }

        return;
    }

    for (size_t i = pageSizeIndexes.StartIndex; i < pageSizeIndexes.EndIndex; i++)
    {
        auto pageInfos = &storage->PagesInfos[i];

        if (IsPageCommitted(storage, (uint32_t)i))
        {
            auto pagePointer = (uint8_t*)storage + storage->HeaderSizeInBytes + i * systemPageSizeInBytes;

            if ((int32_t)(pageInfos->MaxCommittedOffset - pageInfos->MinCommittedOffset) <= 0)
            {
                SystemPlatformDecommitMemory(pagePointer, systemPageSizeInBytes);
                ClearPageCommitted(storage, (uint32_t)i);

                if (needsSynchronization)
                {
                    SystemAtomicSubstract(storage->CommittedPagesCount, 1);
                }
                else
                {
                    storage->CommittedPagesCount--;
                }
            }
        }
    }

    if (needsSynchronization)
    {
        UnlockMemoryArenaCommitOperations(storage);
    }
}

void* SystemPushMemory(MemoryArena memoryArena, size_t sizeInBytes, AllocationState state)
{
    sizeInBytes = SystemAlign(sizeInBytes, MEMORYARENA_DEFAULT_ALIGNMENT);

    auto workingMemoryArena = GetStackWorkingMemoryArena(memoryArena);
    auto storage = workingMemoryArena.Storage;
    uint8_t* pointer;

    if (memoryArena.Storage == stackMemoryArenaStorage)
    {
        auto allocatedSize = GetMemoryArenaAllocatedBytes(workingMemoryArena);

        if (allocatedSize > storage->SizeInBytes || sizeInBytes > storage->SizeInBytes - allocatedSize)
        {
            SystemLogErrorMessage(ElemLogMessageCategory_Memory, "Cannot push to memory arena with: %d (Allocated size is: %d, Max size is: %d)", (uint32_t)sizeInBytes, (uint32_t)allocatedSize, (uint32_t)storage->SizeInBytes);
            return nullptr;
        }

        pointer = storage->CurrentPointer;
        storage->CurrentPointer += sizeInBytes;
    }
    else
    {
        auto dataStart = (uint8_t*)storage + storage->HeaderSizeInBytes;
        SystemAtomicLoad(storage->CurrentPointer, pointer);

        while (true)
        {
            auto allocatedSize = (size_t)(pointer - dataStart);

            if (allocatedSize > storage->SizeInBytes || sizeInBytes > storage->SizeInBytes - allocatedSize)
            {
                SystemLogErrorMessage(ElemLogMessageCategory_Memory, "Cannot push to memory arena with: %d (Allocated size is: %d, Max size is: %d)", (uint32_t)sizeInBytes, (uint32_t)allocatedSize, (uint32_t)storage->SizeInBytes);
                return nullptr;
            }

            auto nextPointer = pointer + sizeInBytes;

            if (SystemAtomicCompareExchange(storage->CurrentPointer, pointer, nextPointer))
            {
                break;
            }

            SystemYieldThread();
        }
    }

    if (state == AllocationState_Committed)
    {
        SystemCommitMemory(workingMemoryArena, pointer, sizeInBytes);
    }

    return pointer;
}

void PopStackMemory(MemoryArena memoryArena, size_t sizeInBytes)
{
    SystemAssert(IsStackMemoryArena(memoryArena));

    auto storage = memoryArena.Storage;
    auto allocatedSize = GetMemoryArenaAllocatedBytes(memoryArena);

    if (sizeInBytes > allocatedSize)
    {
        SystemLogErrorMessage(ElemLogMessageCategory_Memory, "Cannot pop stack memory arena with: %u (Allocated size is: %u)", (uint32_t)sizeInBytes, (uint32_t)allocatedSize);
        return;
    }

    uint8_t* pointer;

    if (memoryArena.Storage == stackMemoryArenaStorage)
    {
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

    if (result == nullptr)
    {
        return nullptr;
    }

    SystemPlatformClearMemory(result, sizeInBytes);
    return result;
}

template<typename T>
Span<T> SystemPushArray(MemoryArena memoryArena, size_t count, AllocationState state)
{
    auto memory = SystemPushMemory(memoryArena, sizeof(T) * count, state);
    return memory ? Span<T>((T*)memory, count) : Span<T>();
}

template<typename T>
Span<T> SystemPushArrayZero(MemoryArena memoryArena, size_t count)
{
    auto memory = SystemPushMemoryZero(memoryArena, sizeof(T) * count);
    return memory ? Span<T>((T*)memory, count) : Span<T>();
}

template<>
Span<char> SystemPushArrayZero<char>(MemoryArena memoryArena, size_t count)
{
    auto memory = SystemPushMemoryZero(memoryArena, sizeof(char) * (count + 1));
    return memory ? Span<char>((char*)memory, count) : Span<char>();
}

template<>
Span<wchar_t> SystemPushArrayZero<wchar_t>(MemoryArena memoryArena, size_t count)
{
    auto memory = SystemPushMemoryZero(memoryArena, sizeof(wchar_t) * (count + 1));
    return memory ? Span<wchar_t>((wchar_t*)memory, count) : Span<wchar_t>();
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

template<>
Span<char> SystemDuplicateBuffer(MemoryArena memoryArena, ReadOnlySpan<char> source)
{
    auto result = SystemPushArrayZero<char>(memoryArena, source.Length);
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