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

struct ShrinkMemoryArenaStorageResult
{
    MemoryArenaStorage* Storage;
    uint32_t StoragesDeleted;
    size_t NewSizeInBytes;
};

thread_local MemoryArenaStorage* stackMemoryArenaStorage = nullptr;
thread_local MemoryArenaStorage* stackMemoryArenaExtraStorage = nullptr;

void SystemReleaseStackMemoryArena(MemoryArena* stackMemoryArena);

StackMemoryArena::~StackMemoryArena()
{
    SystemReleaseStackMemoryArena(MemoryArenaPointer);
}

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
    SystemPlatformFreeMemory(storage->Memory.Pointer);
    SystemPlatformFreeMemory(storage);
}

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

MemoryArena* AllocateMemoryArena(MemoryArenaStorage* storage, size_t startOffset, uint8_t level)
{
    auto result = (MemoryArena*)SystemPlatformAllocateMemory(sizeof(MemoryArena));
    result->Storage = storage;
    result->StartOffset = startOffset;
    result->AllocatedBytes = 0;
    result->SizeInBytes = storage->Memory.Length;
    result->Level = level;
    result->ExtraStorage = nullptr;

    return result;
}

MemoryArena* SystemAllocateMemoryArena()
{
    return SystemAllocateMemoryArena(MEMORYARENA_DEFAULT_SIZE);
}

MemoryArena* SystemAllocateMemoryArena(size_t sizeInBytes)
{
    auto storage = AllocateMemoryArenaStorage(sizeInBytes);
    return AllocateMemoryArena(storage, 0, 0);
}

void SystemFreeMemoryArena(MemoryArena* memoryArena)
{
    FreeMemoryArenaStorage(memoryArena->Storage);
    SystemPlatformFreeMemory(memoryArena);
}

void SystemClearMemoryArena(MemoryArena* memoryArena)
{
    SystemPopMemory(memoryArena, memoryArena->AllocatedBytes);
}

size_t SystemGetMemoryArenaAllocatedBytes(MemoryArena* memoryArena)
{
    return memoryArena->AllocatedBytes;
}

StackMemoryArena SystemGetStackMemoryArena()
{
    if (stackMemoryArenaStorage == nullptr)
    {
        stackMemoryArenaStorage = AllocateMemoryArenaStorage(MEMORYARENA_DEFAULT_SIZE);
        stackMemoryArenaExtraStorage = AllocateMemoryArenaStorage(MEMORYARENA_DEFAULT_SIZE);
    }

    return { AllocateMemoryArena(stackMemoryArenaStorage, stackMemoryArenaStorage->AllocatedBytes, ++stackMemoryArenaStorage->Level)  };
}

void SystemReleaseStackMemoryArena(MemoryArena* stackMemoryArena)
{
    if (stackMemoryArena->ExtraStorage != nullptr)
    {
        SystemPopMemory(stackMemoryArena->ExtraStorage, stackMemoryArena->ExtraStorage->AllocatedBytes);
        SystemPlatformFreeMemory(stackMemoryArena->ExtraStorage);
    }

    stackMemoryArenaStorage->Level--;
    SystemPopMemory(stackMemoryArena, stackMemoryArena->AllocatedBytes);
    SystemPlatformFreeMemory(stackMemoryArena);
}

void* SystemPushMemory(MemoryArena* memoryArena, size_t sizeInBytes)
{
    MemoryArena* workingMemoryArena = memoryArena;

    if (memoryArena->Level !=  memoryArena->Storage->Level)
    {
        if (memoryArena->ExtraStorage == nullptr)
        {
            memoryArena->ExtraStorage = AllocateMemoryArena(stackMemoryArenaExtraStorage, stackMemoryArenaExtraStorage->AllocatedBytes, 0);
        }

        workingMemoryArena = memoryArena->ExtraStorage;
        LogDebugMessage(LogMessageCategory_Memory, L"Using stack memory arena extra storage. (Extra allocated bytes: %llu)", stackMemoryArenaExtraStorage->AllocatedBytes + sizeInBytes);
    }

    auto storage = workingMemoryArena->Storage;

    if (workingMemoryArena->Storage->AllocatedBytes + sizeInBytes > storage->Memory.Length)
    {
        auto oldSizeInBytes = workingMemoryArena->SizeInBytes;
        auto newStorageSize = SystemRoundUpToPowerOf2(max(workingMemoryArena->SizeInBytes, sizeInBytes)); 
        workingMemoryArena->SizeInBytes += newStorageSize;
        workingMemoryArena->AllocatedBytes += storage->Memory.Length - storage->AllocatedBytes;
        storage->AllocatedBytes = storage->Memory.Length;

        LogDebugMessage(LogMessageCategory_Memory, L"Resizing MemoryArena to %llu (Previous size was: %llu) -> SizeInBytes: %llu", workingMemoryArena->SizeInBytes, oldSizeInBytes, sizeInBytes);

        storage = AllocateMemoryArenaStorage(newStorageSize);
        storage->Next = workingMemoryArena->Storage;
        workingMemoryArena->Storage = storage;
    }

    auto result = storage->Memory.Pointer + storage->AllocatedBytes;
    storage->AllocatedBytes += sizeInBytes;
    workingMemoryArena->AllocatedBytes += sizeInBytes;

    return result;
}

void SystemPopMemory(MemoryArena* memoryArena, size_t sizeInBytes)
{
    if (sizeInBytes > memoryArena->AllocatedBytes)
    {
        LogErrorMessage(LogMessageCategory_Memory, L"Cannot pop memory arena with: %llu (Allocated size is: %llu)", sizeInBytes, memoryArena->AllocatedBytes);
        return;
    }

    auto shrinkResult = ShrinkMemoryArenaStorage(memoryArena->Storage, sizeInBytes);
    assert(shrinkResult.Storage);

    auto oldSizeInBytes = memoryArena->SizeInBytes;
    memoryArena->Storage = shrinkResult.Storage;
    memoryArena->AllocatedBytes -= sizeInBytes;
    memoryArena->SizeInBytes = shrinkResult.NewSizeInBytes;

    if (shrinkResult.StoragesDeleted > 0)
    {
        LogDebugMessage(LogMessageCategory_Memory, L"Shrinking MemoryArena storage to %llu (New size: %llu, Previous size was: %llu) -> Deleted Memory Storages: %d, Allocated Bytes: %llu", sizeInBytes, memoryArena->SizeInBytes, oldSizeInBytes, shrinkResult.StoragesDeleted, memoryArena->AllocatedBytes);
    }
}

void* SystemPushMemoryZero(MemoryArena* memoryArena, size_t sizeInBytes)
{
    auto result = SystemPushMemory(memoryArena, sizeInBytes);
    SystemPlatformClearMemory(result, sizeInBytes);

    return result;
}

template<typename T>
Span<T> SystemPushArray(MemoryArena* memoryArena, size_t count)
{
    auto memory = SystemPushMemory(memoryArena, sizeof(T) * count);
    return Span<T>((T*)memory, count);
}

template<typename T>
Span<T> SystemPushArrayZero(MemoryArena* memoryArena, size_t count)
{
    auto memory = SystemPushMemoryZero(memoryArena, sizeof(T) * count);
    return Span<T>((T*)memory, count);
}

template<>
Span<char> SystemPushArrayZero<char>(MemoryArena* memoryArena, size_t count)
{
    auto memory = SystemPushMemoryZero(memoryArena, sizeof(char) * (count + 1));
    return Span<char>((char*)memory, count);
}

template<>
Span<wchar_t> SystemPushArrayZero<wchar_t>(MemoryArena* memoryArena, size_t count)
{
    auto memory = SystemPushMemoryZero(memoryArena, sizeof(wchar_t) * (count + 1));
    return Span<wchar_t>((wchar_t*)memory, count);
}

template<typename T>
T* SystemPushStruct(MemoryArena* memoryArena)
{
    return (T*)SystemPushMemory(memoryArena, sizeof(T));
}

template<typename T>
T* SystemPushStructZero(MemoryArena* memoryArena)
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
Span<T> SystemConcatBuffers(MemoryArena* memoryArena, ReadOnlySpan<T> buffer1, ReadOnlySpan<T> buffer2)
{
    auto result = SystemPushArray<T>(memoryArena, buffer1.Length + buffer2.Length);

    SystemCopyBuffer(result, buffer1);
    SystemCopyBuffer(result.Slice(buffer1.Length), buffer2);

    return result;
}

template<>
Span<char> SystemConcatBuffers(MemoryArena* memoryArena, ReadOnlySpan<char> buffer1, ReadOnlySpan<char> buffer2)
{
    auto result = SystemPushArrayZero<char>(memoryArena, buffer1.Length + buffer2.Length);

    SystemCopyBuffer(result, buffer1);
    SystemCopyBuffer(result.Slice(buffer1.Length), buffer2);

    return result;
}

template<>
Span<wchar_t> SystemConcatBuffers(MemoryArena* memoryArena, ReadOnlySpan<wchar_t> buffer1, ReadOnlySpan<wchar_t> buffer2)
{
    auto result = SystemPushArrayZero<wchar_t>(memoryArena, buffer1.Length + buffer2.Length);

    SystemCopyBuffer(result, buffer1);
    SystemCopyBuffer(result.Slice(buffer1.Length), buffer2);

    return result;
}
