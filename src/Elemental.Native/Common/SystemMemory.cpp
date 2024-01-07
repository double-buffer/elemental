#include "SystemMemory.h"
#include "SystemFunctions.h"
#include "SystemLogging.h"
#include "SystemPlatformFunctions.h"

#define MEMORYARENA_DEFAULT_SIZE 1048576

struct MemoryArenaStorage
{
    Span<uint8_t> Memory;
    size_t AllocatedBytes;
    uint8_t Level;
    MemoryArenaStorage* Next;
};

struct MemoryArenaHandle
{
    MemoryArenaStorage* Storage;
    size_t AllocatedBytes;
    size_t SizeInBytes;
    MemoryArena ExtraStorage;
    uint8_t MinAllocatedLevel;
};

struct ShrinkMemoryArenaStorageResult
{
    MemoryArenaStorage* Storage;
    uint32_t StoragesDeleted;
    size_t NewSizeInBytes;
};

thread_local MemoryArenaHandle* stackMemoryArenaHandle = nullptr;

MemoryArenaStorage* AllocateMemoryArenaStorage(size_t sizeInBytes)
{
    auto pointer = (uint8_t*)SystemPlatformAllocateMemory(sizeInBytes);
    auto memoryArenaStorage = (MemoryArenaStorage*)SystemPlatformAllocateMemory(sizeof(MemoryArenaStorage));
    memoryArenaStorage->Memory = Span<uint8_t>(pointer, sizeInBytes);
    memoryArenaStorage->AllocatedBytes = 0;
    memoryArenaStorage->Level = 0;
    memoryArenaStorage->Next = nullptr;

    return memoryArenaStorage;
}

void FreeMemoryArenaStorage(MemoryArenaStorage* storage)
{
    SystemPlatformFreeMemory(storage->Memory.Pointer, storage->Memory.Length);
    SystemPlatformFreeMemory(storage, sizeof(MemoryArenaStorage));
}

// TODO: We need to find a way to make all the memory functions thread safe
ShrinkMemoryArenaStorageResult ShrinkMemoryArenaStorage(MemoryArenaStorage* storage, size_t sizeInBytes)
{
    size_t newSizeInBytes = 0;
    auto resultStorage = storage;
    auto storageCount = 0;
    auto remainingSizeInBytes = sizeInBytes;

    while (storage != nullptr)
    {
        if (storage->AllocatedBytes < remainingSizeInBytes)
        {
            remainingSizeInBytes -= storage->AllocatedBytes;

            storageCount++;
            auto storageToFree = storage;

            storage = storage->Next;
            FreeMemoryArenaStorage(storageToFree);
        }
        else if (remainingSizeInBytes != 0) 
        {
            storage->AllocatedBytes -= remainingSizeInBytes;
            remainingSizeInBytes = 0;
            resultStorage = storage;
            newSizeInBytes += storage->Memory.Length;
            storage = storage->Next;
        }
        else 
        {
            newSizeInBytes += storage->Memory.Length;
            storage = storage->Next;
        }
    }

    auto result = ShrinkMemoryArenaStorageResult();
    result.Storage = resultStorage;
    result.StoragesDeleted = storageCount;
    result.NewSizeInBytes = newSizeInBytes;

    return result;
}

MemoryArenaHandle* AllocateMemoryArenaHandle(MemoryArenaStorage* storage)
{
    auto handle = (MemoryArenaHandle*)SystemPlatformAllocateMemory(sizeof(MemoryArenaHandle));
    handle->Storage = storage;
    handle->AllocatedBytes = 0;
    handle->SizeInBytes = storage->Memory.Length;
    handle->ExtraStorage = {};
    handle->MinAllocatedLevel = 255;

    return handle;
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
    
    stackMemoryArenaHandle->Storage->Level++;
    auto extraStorageAllocatedBytes = 0llu;

    if (stackMemoryArenaHandle->ExtraStorage.MemoryArenaHandle != nullptr)
    {
        extraStorageAllocatedBytes = stackMemoryArenaHandle->ExtraStorage.MemoryArenaHandle->AllocatedBytes;
    }

    MemoryArena memoryArena = {};
    memoryArena.MemoryArenaHandle = stackMemoryArenaHandle;
    memoryArena.Level = stackMemoryArenaHandle->Storage->Level;

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

    handle->Storage->Level--;

    auto bytesToPop = handle->AllocatedBytes - StartOffsetInBytes;

    if (bytesToPop > 0)
    {
        SystemPopMemory(Arena, bytesToPop);
    }
}

// TODO: Do we need to align memory?
void* SystemPushMemory(MemoryArena memoryArena, size_t sizeInBytes)
{
    MemoryArena workingMemoryArena = memoryArena;

    if (memoryArena.Level != memoryArena.MemoryArenaHandle->Storage->Level)
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

    auto handle = workingMemoryArena.MemoryArenaHandle;
    auto storage = handle->Storage;

    if (handle->Storage->AllocatedBytes + sizeInBytes > storage->Memory.Length)
    {
        auto oldSizeInBytes = handle->SizeInBytes;
        auto oldLevel = storage->Level;
        auto newStorageSize = SystemRoundUpToPowerOf2(SystemMax(handle->SizeInBytes, sizeInBytes));

        handle->SizeInBytes += newStorageSize;
        handle->AllocatedBytes += storage->Memory.Length - storage->AllocatedBytes;
        storage->AllocatedBytes = storage->Memory.Length;

        storage = AllocateMemoryArenaStorage(newStorageSize);
        storage->Level = oldLevel; 
        storage->Next = handle->Storage;
        handle->Storage = storage;
       
        SystemLogDebugMessage(LogMessageCategory_Memory, "Resizing MemoryArena to %u (Previous size was: %u) -> SizeInBytes: %u", handle->SizeInBytes, oldSizeInBytes, sizeInBytes);
    }

    auto result = storage->Memory.Pointer + storage->AllocatedBytes;
    storage->AllocatedBytes += sizeInBytes;
    handle->AllocatedBytes += sizeInBytes;

    return result;
}

void SystemPopMemory(MemoryArena memoryArena, size_t sizeInBytes)
{
    auto handle = memoryArena.MemoryArenaHandle;

    if (sizeInBytes > handle->AllocatedBytes)
    {
        SystemLogErrorMessage(LogMessageCategory_Memory, "Cannot pop memory arena with: %u (Allocated size is: %u)", sizeInBytes, handle->AllocatedBytes);
        return;
    }
    
    auto shrinkResult = ShrinkMemoryArenaStorage(handle->Storage, sizeInBytes);
    assert(shrinkResult.Storage);

    auto oldSizeInBytes = handle->SizeInBytes;
    handle->Storage = shrinkResult.Storage;
    handle->AllocatedBytes -= sizeInBytes;
    handle->SizeInBytes = shrinkResult.NewSizeInBytes;

    if (shrinkResult.StoragesDeleted > 0)
    {
        SystemLogDebugMessage(LogMessageCategory_Memory, "Shrinking MemoryArena storage by popping %u (New size: %u, Previous size was: %u) -> Deleted Memory Storages: %d, Allocated Bytes: %u", sizeInBytes, handle->SizeInBytes, oldSizeInBytes, shrinkResult.StoragesDeleted, handle->AllocatedBytes);
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
