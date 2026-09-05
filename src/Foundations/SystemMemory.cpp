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

void PopStackMemory(MemoryArena memoryArena, size_t sizeInBytes);

size_t GetSystemPageSizeInBytes()
{
    static const size_t pageSizeInBytes = SystemPlatformGetPageSize();
    return pageSizeInBytes;
}

bool TryAlignSize(size_t sizeInBytes, size_t alignment, size_t* result)
{
    auto alignmentMask = alignment - 1;

    if (sizeInBytes > SIZE_MAX - alignmentMask)
    {
        return false;
    }

    *result = (sizeInBytes + alignmentMask) & ~alignmentMask;
    return true;
}

bool TryMultiplySize(size_t value1, size_t value2, size_t* result)
{
    if (value1 != 0 && value2 > SIZE_MAX / value1)
    {
        return false;
    }

    *result = value1 * value2;
    return true;
}

PageSizeIndexes ComputePageSizeInfoIndexes(MemoryArenaStorage* storage, void* pointer, size_t sizeInBytes)
{
    auto pageSizeInBytes = GetSystemPageSizeInBytes();
    auto dataStart = (uint8_t*)storage + storage->HeaderSizeInBytes;
    auto offset = (uint8_t*)pointer - dataStart;
    auto endOffset = offset + sizeInBytes;
    size_t alignedEndOffset;
    auto alignmentSucceeded = TryAlignSize(endOffset, pageSizeInBytes, &alignedEndOffset);
    SystemAssert(alignmentSucceeded);

    PageSizeIndexes result = {};
    result.StartIndex = offset / pageSizeInBytes;
    result.EndIndex = alignedEndOffset / pageSizeInBytes;
    return result;
}

PageSizeIndexes ComputePageSizeLocalOffsets(MemoryArenaStorage* storage, size_t index, void* pointer, size_t sizeInBytes)
{
    auto pageSizeInBytes = GetSystemPageSizeInBytes();
    auto absoluteStart = (uint8_t*)pointer;
    auto absoluteEnd = (uint8_t*)pointer + sizeInBytes;

    auto pageStart = (uint8_t*)storage + storage->HeaderSizeInBytes + index * pageSizeInBytes;
    auto pageEnd = pageStart + pageSizeInBytes;

    PageSizeIndexes result = {};
    result.StartIndex = absoluteStart > pageStart ? absoluteStart - pageStart : 0;
    result.EndIndex = absoluteEnd < pageEnd ? absoluteEnd - pageStart : pageSizeInBytes - 1;

    return result;
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
    auto pageSizeInBytes = GetSystemPageSizeInBytes();
    size_t dataSizeInBytes;

    if (!TryAlignSize(sizeInBytes, pageSizeInBytes, &dataSizeInBytes))
    {
        return nullptr;
    }

    auto pageInfosCount = dataSizeInBytes / pageSizeInBytes;
    auto pageCommitInfosCount = (pageInfosCount + 31) / 32;
    auto headerMetadataSizeInBytes = sizeof(MemoryArenaStorage);

    if (pageInfosCount > (SIZE_MAX - headerMetadataSizeInBytes) / sizeof(MemoryArenaPageInfo))
    {
        return nullptr;
    }

    headerMetadataSizeInBytes += pageInfosCount * sizeof(MemoryArenaPageInfo);

    if (pageCommitInfosCount > (SIZE_MAX - headerMetadataSizeInBytes) / sizeof(MemoryArenaPageCommitInfo))
    {
        return nullptr;
    }

    headerMetadataSizeInBytes += pageCommitInfosCount * sizeof(MemoryArenaPageCommitInfo);

    size_t headerSizeInBytes;

    if (!TryAlignSize(headerMetadataSizeInBytes, pageSizeInBytes, &headerSizeInBytes) ||
        dataSizeInBytes > SIZE_MAX - headerSizeInBytes)
    {
        return nullptr;
    }

    auto reservedSizeInBytes = headerSizeInBytes + dataSizeInBytes;
    auto storage = (MemoryArenaStorage*)SystemPlatformReserveMemory(reservedSizeInBytes);

    if (storage == nullptr)
    {
        return nullptr;
    }

    if (!SystemPlatformCommitMemory(storage, headerSizeInBytes))
    {
        SystemPlatformFreeMemory(storage, reservedSizeInBytes, 0);
        return nullptr;
    }

    storage->CurrentPointer = (uint8_t*)storage + headerSizeInBytes;
    storage->SizeInBytes = sizeInBytes;
    storage->HeaderSizeInBytes = headerSizeInBytes;
    storage->IsCommitOperationInProgres = false;
    storage->CommittedPagesCount = headerSizeInBytes / pageSizeInBytes;
    storage->PagesInfos = (MemoryArenaPageInfo*)((uint8_t*)storage + sizeof(MemoryArenaStorage));
    storage->PagesCommitInfos = (MemoryArenaPageCommitInfo*)((uint8_t*)storage + sizeof(MemoryArenaStorage) + pageInfosCount * sizeof(MemoryArenaPageInfo));
    storage->StackExtraStorage = {};
    storage->StackLevel = 0;
    storage->StackMinAllocatedLevel = 255;

    for (size_t i = 0; i < pageInfosCount; i++)
    {
        ClearPageCommitted(storage, (uint32_t)i);
        storage->PagesInfos[i].MinCommittedOffset = pageSizeInBytes - 1;
        storage->PagesInfos[i].MaxCommittedOffset = 0;
    }
    
    return storage;
}

MemoryArena GetStackWorkingMemoryArena(MemoryArena memoryArena)
{
    if (memoryArena.Storage == nullptr)
    {
        return {};
    }

    MemoryArena workingMemoryArena = memoryArena;

    if (memoryArena.Level != memoryArena.Storage->StackLevel)
    {
        if (memoryArena.Storage->StackExtraStorage.Storage == nullptr)
        {
            auto extraStorage = AllocateMemoryArenaStorage(MEMORYARENA_DEFAULT_SIZE);

            if (extraStorage == nullptr)
            {
                return {};
            }

            memoryArena.Storage->StackExtraStorage = { extraStorage, 0 };
        }

        workingMemoryArena = memoryArena.Storage->StackExtraStorage;
        memoryArena.Storage->StackMinAllocatedLevel = SystemMin(memoryArena.Storage->StackMinAllocatedLevel, memoryArena.Level);
    }

    return workingMemoryArena;
}

bool IsStackMemoryArena(MemoryArena memoryArena)
{
    if (memoryArena.Storage == nullptr || stackMemoryArenaStorage == nullptr)
    {
        return false;
    }

    return memoryArena.Storage == stackMemoryArenaStorage ||
        (stackMemoryArenaStorage->StackExtraStorage.Storage != nullptr && memoryArena.Storage == stackMemoryArenaStorage->StackExtraStorage.Storage);
}

size_t GetMemoryArenaAllocatedBytes(MemoryArena memoryArena)
{
    if (memoryArena.Storage == nullptr)
    {
        return 0;
    }

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
    if (memoryArena.Storage == nullptr)
    {
        return;
    }

    auto pageSizeInBytes = GetSystemPageSizeInBytes();
    size_t dataSizeInBytes;
    auto alignmentSucceeded = TryAlignSize(memoryArena.Storage->SizeInBytes, pageSizeInBytes, &dataSizeInBytes);
    SystemAssert(alignmentSucceeded);

    auto reservedSizeInBytes = memoryArena.Storage->HeaderSizeInBytes + dataSizeInBytes;
    auto committedSizeInBytes = memoryArena.Storage->CommittedPagesCount * pageSizeInBytes;
    SystemPlatformFreeMemory(memoryArena.Storage, reservedSizeInBytes, committedSizeInBytes);
}

void SystemClearMemoryArena(MemoryArena memoryArena)
{
    if (memoryArena.Storage == nullptr)
    {
        return;
    }

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
    if (memoryArena.Storage == nullptr)
    {
        return {};
    }

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
    result.CommittedBytes = committedPagesCount * GetSystemPageSizeInBytes();
    result.MaximumSizeInBytes = memoryArena.Storage->SizeInBytes;

    return result;
}

StackMemoryArena SystemGetStackMemoryArena()
{
    if (stackMemoryArenaStorage == nullptr)
    {
        stackMemoryArenaStorage = AllocateMemoryArenaStorage(MEMORYARENA_DEFAULT_SIZE);

        if (stackMemoryArenaStorage == nullptr)
        {
            return {};
        }
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
    if (Arena.Storage == nullptr)
    {
        return;
    }

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
bool SystemCommitMemory(MemoryArena memoryArena, ReadOnlySpan<T> buffer, bool clearMemory)
{
    size_t sizeInBytes;

    if (!TryMultiplySize(sizeof(T), buffer.Length, &sizeInBytes))
    {
        return false;
    }

    return SystemCommitMemory(memoryArena, (void*)buffer.Pointer, sizeInBytes, clearMemory);
}

bool SystemCommitMemory(MemoryArena memoryArena, void* pointer, size_t sizeInBytes, bool clearMemory)
{
    if (sizeInBytes == 0)
    {
        return true;
    }

    if (memoryArena.Storage == nullptr || pointer == nullptr)
    {
        return false;
    }

    auto storage = memoryArena.Storage;
    auto dataStart = (uintptr_t)storage + storage->HeaderSizeInBytes;
    auto pointerAddress = (uintptr_t)pointer;

    if (pointerAddress < dataStart)
    {
        return false;
    }

    auto offset = (size_t)(pointerAddress - dataStart);

    if (offset > storage->SizeInBytes || sizeInBytes > storage->SizeInBytes - offset)
    {
        return false;
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

        return true;
    }

    auto pageSizeInBytes = GetSystemPageSizeInBytes();

    for (size_t i = pageSizeIndexes.StartIndex; i < pageSizeIndexes.EndIndex; i++)
    {
        if (!IsPageCommitted(storage, (uint32_t)i))
        {
            auto pagePointer = (uint8_t*)storage + storage->HeaderSizeInBytes + i * pageSizeInBytes;

            if (!SystemPlatformCommitMemory(pagePointer, pageSizeInBytes))
            {
                SystemLogErrorMessage(ElemLogMessageCategory_Memory, "Cannot commit memory arena page.");

                if (needsSynchronization)
                {
                    UnlockMemoryArenaCommitOperations(storage);
                }

                return false;
            }
            
            if (clearMemory)
            {
                SystemPlatformClearMemory(pagePointer, pageSizeInBytes);
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

    return true;
}

void SystemDecommitMemory(MemoryArena memoryArena, void* pointer, size_t sizeInBytes)
{
    if (memoryArena.Storage == nullptr || pointer == nullptr || sizeInBytes == 0)
    {
        return;
    }

    auto storage = memoryArena.Storage;
    auto dataStart = (uintptr_t)storage + storage->HeaderSizeInBytes;
    auto pointerAddress = (uintptr_t)pointer;

    if (pointerAddress < dataStart)
    {
        return;
    }

    auto offset = (size_t)(pointerAddress - dataStart);

    if (offset > storage->SizeInBytes || sizeInBytes > storage->SizeInBytes - offset)
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

    auto pageSizeInBytes = GetSystemPageSizeInBytes();

    for (size_t i = pageSizeIndexes.StartIndex; i < pageSizeIndexes.EndIndex; i++)
    {
        auto pageInfos = &storage->PagesInfos[i];

        if (IsPageCommitted(storage, (uint32_t)i) && (int32_t)(pageInfos->MaxCommittedOffset - pageInfos->MinCommittedOffset) <= 0)
        {
            auto pagePointer = (uint8_t*)storage + storage->HeaderSizeInBytes + i * pageSizeInBytes;

            if (SystemPlatformDecommitMemory(pagePointer, pageSizeInBytes))
            {
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
    if (memoryArena.Storage == nullptr)
    {
        return nullptr;
    }

    size_t alignedSizeInBytes;

    if (!TryAlignSize(sizeInBytes, MEMORYARENA_DEFAULT_ALIGNMENT, &alignedSizeInBytes))
    {
        return nullptr;
    }

    sizeInBytes = alignedSizeInBytes;

    auto workingMemoryArena = GetStackWorkingMemoryArena(memoryArena);

    if (workingMemoryArena.Storage == nullptr)
    {
        return nullptr;
    }

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

    if (state == AllocationState_Committed && !SystemCommitMemory(workingMemoryArena, pointer, sizeInBytes))
    {
        if (IsStackMemoryArena(workingMemoryArena))
        {
            storage->CurrentPointer -= sizeInBytes;
        }

        return nullptr;
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
    size_t sizeInBytes;

    if (!TryMultiplySize(sizeof(T), count, &sizeInBytes))
    {
        return {};
    }

    auto memory = SystemPushMemory(memoryArena, sizeInBytes, state);
    return memory ? Span<T>((T*)memory, count) : Span<T>();
}

template<typename T>
Span<T> SystemPushArrayZero(MemoryArena memoryArena, size_t count)
{
    size_t sizeInBytes;

    if (!TryMultiplySize(sizeof(T), count, &sizeInBytes))
    {
        return {};
    }

    auto memory = SystemPushMemoryZero(memoryArena, sizeInBytes);
    return memory ? Span<T>((T*)memory, count) : Span<T>();
}

template<>
Span<char> SystemPushArrayZero<char>(MemoryArena memoryArena, size_t count)
{
    if (count == SIZE_MAX)
    {
        return {};
    }

    auto memory = SystemPushMemoryZero(memoryArena, count + 1);
    return memory ? Span<char>((char*)memory, count) : Span<char>();
}

template<>
Span<wchar_t> SystemPushArrayZero<wchar_t>(MemoryArena memoryArena, size_t count)
{
    if (count == SIZE_MAX)
    {
        return {};
    }

    size_t sizeInBytes;

    if (!TryMultiplySize(sizeof(wchar_t), count + 1, &sizeInBytes))
    {
        return {};
    }

    auto memory = SystemPushMemoryZero(memoryArena, sizeInBytes);
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

    if (source.Length == 0)
    {
        return;
    }

    size_t sizeInBytes;

    if (!TryMultiplySize(sizeof(T), source.Length, &sizeInBytes))
    {
        return;
    }

    SystemPlatformCopyMemory(destination.Pointer, source.Pointer, sizeInBytes);
}

template<typename T>
Span<T> SystemDuplicateBuffer(MemoryArena memoryArena, ReadOnlySpan<T> source)
{
    auto result = SystemPushArray<T>(memoryArena, source.Length);

    if (result.Pointer == nullptr)
    {
        return {};
    }

    SystemCopyBuffer(result, source);
    return result;
}

template<>
Span<char> SystemDuplicateBuffer(MemoryArena memoryArena, ReadOnlySpan<char> source)
{
    auto result = SystemPushArrayZero<char>(memoryArena, source.Length);

    if (result.Pointer == nullptr)
    {
        return {};
    }

    SystemCopyBuffer(result, source);
    return result;
}

template<typename T>
Span<T> SystemConcatBuffers(MemoryArena memoryArena, ReadOnlySpan<T> buffer1, ReadOnlySpan<T> buffer2)
{
    if (buffer1.Length > SIZE_MAX - buffer2.Length)
    {
        return {};
    }

    auto result = SystemPushArray<T>(memoryArena, buffer1.Length + buffer2.Length);

    if (result.Pointer == nullptr)
    {
        return {};
    }

    SystemCopyBuffer(result, buffer1);
    SystemCopyBuffer(result.Slice(buffer1.Length), buffer2);

    return result;
}

template<>
Span<char> SystemConcatBuffers(MemoryArena memoryArena, ReadOnlySpan<char> buffer1, ReadOnlySpan<char> buffer2)
{
    if (buffer1.Length > SIZE_MAX - buffer2.Length)
    {
        return {};
    }

    auto result = SystemPushArrayZero<char>(memoryArena, buffer1.Length + buffer2.Length);

    if (result.Pointer == nullptr)
    {
        return {};
    }

    SystemCopyBuffer(result, buffer1);
    SystemCopyBuffer(result.Slice(buffer1.Length), buffer2);

    return result;
}

template<>
Span<wchar_t> SystemConcatBuffers(MemoryArena memoryArena, ReadOnlySpan<wchar_t> buffer1, ReadOnlySpan<wchar_t> buffer2)
{
    if (buffer1.Length > SIZE_MAX - buffer2.Length)
    {
        return {};
    }

    auto result = SystemPushArrayZero<wchar_t>(memoryArena, buffer1.Length + buffer2.Length);

    if (result.Pointer == nullptr)
    {
        return {};
    }

    SystemCopyBuffer(result, buffer1);
    SystemCopyBuffer(result.Slice(buffer1.Length), buffer2);

    return result;
}
