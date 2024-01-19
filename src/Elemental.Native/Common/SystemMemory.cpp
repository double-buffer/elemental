#include "SystemMemory.h"
#include "SystemFunctions.h"
#include "SystemLogging.h"
#include "SystemPlatformFunctions.h"

#define MEMORYARENA_DEFAULT_SIZE 64 * 1024 * 1024

struct MemoryArenaStorage
{
    uint8_t* CurrentPointer;
    uint8_t* CommitedMemoryPointer;
    size_t SizeInBytes;
    bool IsCommitOperationInProgres;

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

MemoryArenaStorage* AllocateMemoryArenaStorage(size_t sizeInBytes)
{
    if (systemPageSizeInBytes == 0)
    {
        systemPageSizeInBytes = SystemPlatformGetPageSize();
    }

    auto sizeResized = ResizeToPageSizeMultiple(sizeof(MemoryArenaStorage) + sizeInBytes, systemPageSizeInBytes);
    auto storage = (MemoryArenaStorage*)SystemPlatformReserveMemory(sizeResized);
    SystemPlatformCommitMemory(storage, systemPageSizeInBytes);

    storage->CurrentPointer = (uint8_t*)storage + sizeof(MemoryArenaStorage);
    storage->CommitedMemoryPointer = (uint8_t*)storage + systemPageSizeInBytes;
    storage->SizeInBytes = sizeInBytes;
    storage->IsCommitOperationInProgres = false;
    storage->ExtraStorage = {};
    storage->Level = 0;
    storage->MinAllocatedLevel = 255;

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
    SystemPlatformFreeMemory(memoryArena.Storage, sizeof(MemoryArenaStorage) + memoryArena.Storage->SizeInBytes);
}

void SystemClearMemoryArena(MemoryArena memoryArena)
{
    SystemPopMemory(memoryArena, SystemGetMemoryArenaAllocatedBytes(memoryArena));
}

size_t SystemGetMemoryArenaAllocatedBytes(MemoryArena memoryArena)
{
    return memoryArena.Storage->CurrentPointer - (uint8_t*)memoryArena.Storage - sizeof(MemoryArenaStorage);
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

void* SystemPushMemory(MemoryArena memoryArena, size_t sizeInBytes)
{
    // TODO: Don't use atomics if the memory arena equals stack memory arena

    // TODO: Align sizeInBytes
    auto storage = GetStackWorkingMemoryArena(memoryArena).Storage;
    auto allocatedSize = SystemGetMemoryArenaAllocatedBytes(memoryArena);

    if (allocatedSize + sizeInBytes > storage->SizeInBytes)
    {
        SystemLogErrorMessage(LogMessageCategory_Memory, "Memory Arena size limit!!! (TODO: Message)");
        return nullptr;
    }

    auto pointer = __atomic_fetch_add(&storage->CurrentPointer, sizeInBytes, __ATOMIC_SEQ_CST);

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

            SystemPlatformCommitMemory(storage->CommitedMemoryPointer, sizeResized);
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
