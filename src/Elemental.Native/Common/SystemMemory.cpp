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

thread_local MemoryArena* stackMemoryArena = nullptr;

void SystemReleaseStackMemoryArena(MemoryArena* stackMemoryArena, size_t startOffsetInBytes, size_t startExtraOffsetInBytes);

StackMemoryArena::~StackMemoryArena()
{
    SystemReleaseStackMemoryArena(MemoryArenaPointer, StartOffsetInBytes, StartExtraOffsetInBytes);
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

MemoryArena* AllocateMemoryArena(MemoryArenaStorage* storage)
{
    auto result = (MemoryArena*)SystemPlatformAllocateMemory(sizeof(MemoryArena));
    result->Storage = storage;
    result->AllocatedBytes = 0;
    result->SizeInBytes = storage->Memory.Length;
    result->Level = 0;
    result->ExtraStorage = nullptr;
    result->MinAllocatedLevel = 255;

    return result;
}

MemoryArena* SystemAllocateMemoryArena()
{
    return SystemAllocateMemoryArena(MEMORYARENA_DEFAULT_SIZE);
}

MemoryArena* SystemAllocateMemoryArena(size_t sizeInBytes)
{
    auto storage = AllocateMemoryArenaStorage(sizeInBytes);
    return AllocateMemoryArena(storage);
}

void SystemFreeMemoryArena(MemoryArena* memoryArena)
{
    FreeMemoryArenaStorage(memoryArena->Storage);
    SystemPlatformFreeMemory(memoryArena, sizeof(MemoryArena));
}

void SystemClearMemoryArena(MemoryArena* memoryArena)
{
    SystemPopMemory(memoryArena, memoryArena->AllocatedBytes);
}

size_t SystemGetMemoryArenaAllocatedBytes(MemoryArena* memoryArena)
{
    return memoryArena->AllocatedBytes;
}

#include <stdio.h>
StackMemoryArena SystemGetStackMemoryArena()
{
    if (stackMemoryArena == nullptr)
    {
        auto stackMemoryArenaStorage = AllocateMemoryArenaStorage(MEMORYARENA_DEFAULT_SIZE);
        stackMemoryArena = AllocateMemoryArena(stackMemoryArenaStorage);
    }
    
    stackMemoryArena->Storage->Level++;
    auto extraStorageAllocatedBytes = 0llu;

    if (stackMemoryArena->ExtraStorage != nullptr)
    {
        extraStorageAllocatedBytes = stackMemoryArena->ExtraStorage->AllocatedBytes;
    }
    printf("Stack ARENA %llu: %llu %d\n", stackMemoryArena, stackMemoryArena->AllocatedBytes, stackMemoryArena->Storage->Level);
    return { stackMemoryArena, stackMemoryArena->Storage->Level, stackMemoryArena->AllocatedBytes, extraStorageAllocatedBytes };
}

void SystemReleaseStackMemoryArena(MemoryArena* stackMemoryArena, size_t startOffsetInBytes, size_t startExtraOffsetInBytes)
{
    printf("Release STACK: ExtraMinLevel=%d, CurrentLevel=%d\n", stackMemoryArena->MinAllocatedLevel, stackMemoryArena->Level);
    if (stackMemoryArena->ExtraStorage != nullptr)
    {
        if (stackMemoryArena->ExtraStorage->AllocatedBytes > 0 && stackMemoryArena->MinAllocatedLevel >= stackMemoryArena->Level)
        {
            SystemPopMemory(stackMemoryArena->ExtraStorage, stackMemoryArena->ExtraStorage->AllocatedBytes - startExtraOffsetInBytes);
            stackMemoryArena->MinAllocatedLevel = 255;
        } 
    }

    stackMemoryArena->Storage->Level--;
    SystemPopMemory(stackMemoryArena, stackMemoryArena->AllocatedBytes - startOffsetInBytes);
}

// TODO: Do we need to align memory?
void* SystemPushMemory(MemoryArena* memoryArena, size_t sizeInBytes)
{
    MemoryArena* workingMemoryArena = memoryArena;

    if (memoryArena->Level != memoryArena->Storage->Level)
    {
        if (memoryArena->ExtraStorage == nullptr)
        {
            auto stackMemoryArenaExtraStorage = AllocateMemoryArenaStorage(MEMORYARENA_DEFAULT_SIZE);
            memoryArena->ExtraStorage = AllocateMemoryArena(stackMemoryArenaExtraStorage);
        }

        workingMemoryArena = memoryArena->ExtraStorage;

        printf("CurrentExtraLevel: %d (Storage: %d)\n", memoryArena->Level, memoryArena->Storage->Level);
        memoryArena->MinAllocatedLevel = SystemMin(memoryArena->MinAllocatedLevel, memoryArena->Level);
    }

    auto storage = workingMemoryArena->Storage;

    printf("Push %llu: %llu => %llu\n", sizeInBytes, workingMemoryArena, workingMemoryArena->AllocatedBytes);

    if (workingMemoryArena->Storage->AllocatedBytes + sizeInBytes > storage->Memory.Length)
    {
        auto oldSizeInBytes = workingMemoryArena->SizeInBytes;
        auto oldLevel = storage->Level;
        auto newStorageSize = SystemRoundUpToPowerOf2(SystemMax(workingMemoryArena->SizeInBytes, sizeInBytes)); 
        workingMemoryArena->SizeInBytes += newStorageSize;
        workingMemoryArena->AllocatedBytes += storage->Memory.Length - storage->AllocatedBytes;
        storage->AllocatedBytes = storage->Memory.Length;

        storage = AllocateMemoryArenaStorage(newStorageSize);
        storage->Level = oldLevel; 
        storage->Next = workingMemoryArena->Storage;
        workingMemoryArena->Storage = storage;
       
        SystemLogDebugMessage(LogMessageCategory_Memory, "Resizing MemoryArena to %u (Previous size was: %u) -> SizeInBytes: %u", workingMemoryArena->SizeInBytes, oldSizeInBytes, sizeInBytes);
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
        SystemLogErrorMessage(LogMessageCategory_Memory, "Cannot pop memory arena with: %u (Allocated size is: %u)", sizeInBytes, memoryArena->AllocatedBytes);
        return;
    }
    
    printf("Pop %llu: %llu => %llu\n", sizeInBytes, memoryArena, memoryArena->AllocatedBytes);

    auto shrinkResult = ShrinkMemoryArenaStorage(memoryArena->Storage, sizeInBytes);
    assert(shrinkResult.Storage);

    auto oldSizeInBytes = memoryArena->SizeInBytes;
    memoryArena->Storage = shrinkResult.Storage;
    memoryArena->AllocatedBytes -= sizeInBytes;
    memoryArena->SizeInBytes = shrinkResult.NewSizeInBytes;

    if (shrinkResult.StoragesDeleted > 0)
    {
        SystemLogDebugMessage(LogMessageCategory_Memory, "Shrinking MemoryArena storage by popping %u (New size: %u, Previous size was: %u) -> Deleted Memory Storages: %d, Allocated Bytes: %u", sizeInBytes, memoryArena->SizeInBytes, oldSizeInBytes, shrinkResult.StoragesDeleted, memoryArena->AllocatedBytes);
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
