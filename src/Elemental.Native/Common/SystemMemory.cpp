#include "SystemMemory.h"
#include "SystemFunctions.h"
#include "SystemLogging.h"
#include "SystemPlatformFunctions.h"

#define MEMORYARENA_DEFAULT_SIZE 1048576

struct MemoryArenaStorage
{
    Span<uint8_t> Memory;
    uint8_t* CurrentPointer;
    MemoryArenaStorage* Next;
};

struct MemoryArenaHandle
{
    MemoryArenaStorage* Storage;
    size_t AllocatedBytes;
    size_t SizeInBytes;
    MemoryArena ExtraStorage;
    uint8_t Level;
    uint8_t MinAllocatedLevel;
};

thread_local MemoryArenaHandle* stackMemoryArenaHandle = nullptr;

MemoryArenaStorage* AllocateMemoryArenaStorage(size_t sizeInBytes)
{
    // TODO: Do only one allocation here to maximize page size 
    auto pointer = (uint8_t*)SystemPlatformReserveMemory(sizeInBytes);
    SystemPlatformCommitMemory(pointer, sizeInBytes);

    auto memoryArenaStorage = (MemoryArenaStorage*)SystemPlatformReserveMemory(sizeof(MemoryArenaStorage));
    SystemPlatformCommitMemory(memoryArenaStorage, sizeof(MemoryArenaStorage));
    
    memoryArenaStorage->Memory = Span<uint8_t>(pointer, sizeInBytes);
    memoryArenaStorage->CurrentPointer = memoryArenaStorage->Memory.Pointer;
    memoryArenaStorage->Next = nullptr;

    return memoryArenaStorage;
}

void FreeMemoryArenaStorage(MemoryArenaStorage* storage)
{
    SystemPlatformFreeMemory(storage->Memory.Pointer, storage->Memory.Length);
    SystemPlatformFreeMemory(storage, sizeof(MemoryArenaStorage));
}

MemoryArenaHandle* AllocateMemoryArenaHandle(MemoryArenaStorage* storage)
{
    auto handle = (MemoryArenaHandle*)SystemPlatformReserveMemory(sizeof(MemoryArenaHandle));
    SystemPlatformCommitMemory(handle, sizeof(MemoryArenaHandle));

    handle->Storage = storage;
    handle->AllocatedBytes = 0;
    handle->SizeInBytes = storage->Memory.Length;
    handle->ExtraStorage = {};
    handle->Level = 0;
    handle->MinAllocatedLevel = 255;

    return handle;
}

MemoryArena GetStackWorkingMemoryArena(MemoryArena memoryArena)
{
    MemoryArena workingMemoryArena = memoryArena;

    if (memoryArena.Level != memoryArena.MemoryArenaHandle->Level)
    {
        if (memoryArena.MemoryArenaHandle->ExtraStorage.MemoryArenaHandle == nullptr)
        {
            auto stackMemoryArenaExtraStorage = AllocateMemoryArenaStorage(MEMORYARENA_DEFAULT_SIZE);
            auto extraHandle = AllocateMemoryArenaHandle(stackMemoryArenaExtraStorage);
            memoryArena.MemoryArenaHandle->ExtraStorage = { extraHandle, 0 };
        }

        workingMemoryArena = memoryArena.MemoryArenaHandle->ExtraStorage;
        memoryArena.MemoryArenaHandle->MinAllocatedLevel = SystemMin(memoryArena.MemoryArenaHandle->MinAllocatedLevel, memoryArena.Level);
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
    auto handle = AllocateMemoryArenaHandle(storage);

    MemoryArena result = {};
    result.MemoryArenaHandle = handle;
    result.Level = 0;
    return result;
}

void SystemFreeMemoryArena(MemoryArena memoryArena)
{
    FreeMemoryArenaStorage(memoryArena.MemoryArenaHandle->Storage);
    SystemPlatformFreeMemory(memoryArena.MemoryArenaHandle, sizeof(MemoryArenaHandle));
}

void SystemClearMemoryArena(MemoryArena memoryArena)
{
    SystemPopMemory(memoryArena, memoryArena.MemoryArenaHandle->AllocatedBytes);
}

size_t SystemGetMemoryArenaAllocatedBytes(MemoryArena memoryArena)
{
    return memoryArena.MemoryArenaHandle->AllocatedBytes;
}

StackMemoryArena SystemGetStackMemoryArena()
{
    if (stackMemoryArenaHandle == nullptr)
    {
        auto stackMemoryArenaStorage = AllocateMemoryArenaStorage(MEMORYARENA_DEFAULT_SIZE);
        stackMemoryArenaHandle = AllocateMemoryArenaHandle(stackMemoryArenaStorage);
    }
    
    stackMemoryArenaHandle->Level++;
    auto extraStorageAllocatedBytes = 0llu;

    if (stackMemoryArenaHandle->ExtraStorage.MemoryArenaHandle != nullptr)
    {
        extraStorageAllocatedBytes = stackMemoryArenaHandle->ExtraStorage.MemoryArenaHandle->AllocatedBytes;
    }

    MemoryArena memoryArena = {};
    memoryArena.MemoryArenaHandle = stackMemoryArenaHandle;
    memoryArena.Level = stackMemoryArenaHandle->Level;

    return { memoryArena, stackMemoryArenaHandle->AllocatedBytes, extraStorageAllocatedBytes };
}

StackMemoryArena::~StackMemoryArena()
{
    auto handle = Arena.MemoryArenaHandle;

    if (handle->ExtraStorage.MemoryArenaHandle != nullptr)
    {
        auto extraBytesToPop = handle->ExtraStorage.MemoryArenaHandle->AllocatedBytes - StartExtraOffsetInBytes;

        if (extraBytesToPop && handle->MinAllocatedLevel >= Arena.Level)
        {
            SystemPopMemory(handle->ExtraStorage, extraBytesToPop);
            handle->MinAllocatedLevel = 255;
        } 
    }

    handle->Level--;

    auto bytesToPop = handle->AllocatedBytes - StartOffsetInBytes;

    if (bytesToPop > 0)
    {
        SystemPopMemory(Arena, bytesToPop);
    }
}

void* SystemPushMemory(MemoryArena memoryArena, size_t sizeInBytes)
{
    // TODO: Don't use atomics if the memory arena equals stack memory arena

    // TODO: Align sizeInBytes
    auto handle = GetStackWorkingMemoryArena(memoryArena).MemoryArenaHandle;
    auto storage = handle->Storage;
    
    auto allocatedSize = (size_t)((size_t)storage->CurrentPointer - (size_t)storage->Memory.Pointer);

    // TODO: This should be locked with a spinwait mutex style
    // TODO: Should we use an exchange here because the allocated size can change between threads

    if (allocatedSize + sizeInBytes > storage->Memory.Length)
    {
        auto oldSizeInBytes = handle->SizeInBytes;
        auto oldLevel = handle->Level;
        auto newStorageSize = SystemRoundUpToPowerOf2(SystemMax(handle->SizeInBytes, sizeInBytes));

        handle->SizeInBytes += newStorageSize;
        handle->AllocatedBytes += (storage->Memory.Pointer + storage->Memory.Length) - storage->CurrentPointer;
        storage->CurrentPointer = storage->Memory.Pointer + storage->Memory.Length;

        storage = AllocateMemoryArenaStorage(newStorageSize);
        handle->Level = oldLevel; 
        storage->Next = handle->Storage;
        handle->Storage = storage;
       
        SystemLogDebugMessage(LogMessageCategory_Memory, "Resizing MemoryArena to %u (Previous size was: %u) -> SizeInBytes: %u", handle->SizeInBytes, oldSizeInBytes, sizeInBytes);
    }

    __atomic_fetch_add(&handle->AllocatedBytes, sizeInBytes, __ATOMIC_SEQ_CST);
    return __atomic_fetch_add(&storage->CurrentPointer, sizeInBytes, __ATOMIC_SEQ_CST);
}

void SystemPopMemory(MemoryArena memoryArena, size_t sizeInBytes)
{
    auto handle = memoryArena.MemoryArenaHandle;
    auto storage = handle->Storage;

    if (sizeInBytes > handle->AllocatedBytes)
    {
        SystemLogErrorMessage(LogMessageCategory_Memory, "Cannot pop memory arena with: %u (Allocated size is: %u)", sizeInBytes, handle->AllocatedBytes);
        return;
    }
    
    MemoryArenaStorage* storagesToFree[255];
    auto storagesToFreeCount = 0;
    
    size_t newSizeInBytes = 0;
    auto resultStorage = storage;
    auto remainingSizeInBytes = sizeInBytes;

    while (storage != nullptr)
    {
        auto storageAllocatedBytes = (size_t)(storage->CurrentPointer - storage->Memory.Pointer);

        if (storageAllocatedBytes < remainingSizeInBytes)
        {
            remainingSizeInBytes -= storageAllocatedBytes;
            storagesToFree[storagesToFreeCount++] = storage;
        }
        else if (remainingSizeInBytes != 0) 
        {
            storage->CurrentPointer -= remainingSizeInBytes;
            remainingSizeInBytes = 0;
            resultStorage = storage;
            newSizeInBytes += storage->Memory.Length;
        }
        else 
        {
            newSizeInBytes += storage->Memory.Length;
        }

        storage = storage->Next;
    }

    assert(resultStorage);

    auto oldSizeInBytes = handle->SizeInBytes;
    handle->Storage = resultStorage;
    handle->AllocatedBytes -= sizeInBytes;
    handle->SizeInBytes = newSizeInBytes;

    if (storagesToFreeCount > 0)
    {
        SystemLogDebugMessage(LogMessageCategory_Memory, "Shrinking MemoryArena storage by popping %u (New size: %u, Previous size was: %u) -> Deleted Memory Storages: %d, Allocated Bytes: %u", sizeInBytes, handle->SizeInBytes, oldSizeInBytes, storagesToFreeCount, handle->AllocatedBytes);

        for (int32_t i = 0; i < storagesToFreeCount; i++)
        {
            FreeMemoryArenaStorage(storagesToFree[i]);
        }
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
