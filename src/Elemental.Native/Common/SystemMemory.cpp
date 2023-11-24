#include "SystemMemory.h"

// TODO: Remove: Temporary
#include <stdio.h>

struct MemoryArenaStorage
{
    Span<uint8_t> Memory;
    size_t AllocatedBytes;
    uint8_t Level;
};

thread_local MemoryArenaStorage* stackMemoryArenaStorage = nullptr;
thread_local MemoryArenaStorage* stackMemoryArenaExtraStorage = nullptr;

MemoryArenaStorage* SystemAllocateMemoryArenaStorage(size_t sizeInBytes)
{
    // TODO: Review the mallocs?
    auto pointer = (uint8_t*)malloc(sizeInBytes); // TODO: System call to review
    auto memoryArenaStorage = (MemoryArenaStorage*)malloc(sizeof(MemoryArenaStorage));
    memoryArenaStorage->Memory = Span<uint8_t>(pointer, sizeInBytes);
    memoryArenaStorage->AllocatedBytes = 0;
    memoryArenaStorage->Level = 0;

    return memoryArenaStorage;
}

MemoryArena* SystemAllocateMemoryArena(size_t sizeInBytes)
{
    auto memoryArenaStorage = SystemAllocateMemoryArenaStorage(sizeInBytes);

    // TODO: Review the mallocs?
    auto result = (MemoryArena*)malloc(sizeof(MemoryArena)); // TODO: System call to review
    result->MemoryArenaStorage = memoryArenaStorage;
    result->StartOffset = 0;
    result->Level = 0;
    result->ExtraStorage = nullptr;

    return result;
}

void SystemFreeMemoryArena(MemoryArena* memoryArena)
{
    free(memoryArena->MemoryArenaStorage->Memory.Pointer); // TODO: System call to review
    free(memoryArena->MemoryArenaStorage);
    free(memoryArena); // TODO: System call to review
}

void SystemClearMemoryArena(MemoryArena* memoryArena)
{
    memoryArena->MemoryArenaStorage->AllocatedBytes = 0;
}

size_t SystemGetMemoryArenaAllocatedBytes(MemoryArena* memoryArena)
{
    return memoryArena->MemoryArenaStorage->AllocatedBytes;
}

StackMemoryArena SystemGetStackMemoryArena()
{
    if (stackMemoryArenaStorage == nullptr)
    {
        stackMemoryArenaStorage = SystemAllocateMemoryArenaStorage(1024);
        stackMemoryArenaExtraStorage = SystemAllocateMemoryArenaStorage(1024);
    }

    // TODO: Review the mallocs? And Refactor to a common function
    auto result = (MemoryArena*)malloc(sizeof(MemoryArena)); // TODO: System call to review
    result->MemoryArenaStorage = stackMemoryArenaStorage;
    result->StartOffset = stackMemoryArenaStorage->AllocatedBytes;
    result->Level = ++stackMemoryArenaStorage->Level;
    result->ExtraStorage = nullptr;

    return { result };
}

StackMemoryArena::~StackMemoryArena()
{
    stackMemoryArenaStorage->Level--;
    stackMemoryArenaStorage->AllocatedBytes = MemoryArenaPointer->StartOffset;

    if (MemoryArenaPointer->ExtraStorage != nullptr)
    {
        stackMemoryArenaExtraStorage->AllocatedBytes = MemoryArenaPointer->ExtraStorage->StartOffset;
        // TODO: Refactor
        free(MemoryArenaPointer->ExtraStorage);
    }

    // TODO: Refactor
    free(MemoryArenaPointer);
}

void* SystemPushMemory(MemoryArena* memoryArena, size_t sizeInBytes)
{
    // TODO: Add checks
    // TODO: Grows the arena if needed

    if (stackMemoryArenaExtraStorage != nullptr)
    {
        printf("ExtraStorage: %llu\n", stackMemoryArenaExtraStorage->AllocatedBytes);
    }

    MemoryArena* workingMemoryArena = memoryArena;

    if (memoryArena->Level !=  memoryArena->MemoryArenaStorage->Level)
    {
        printf("PROBLEM: %d - %d\n", memoryArena->Level, memoryArena->MemoryArenaStorage->Level);

        if (memoryArena->ExtraStorage == nullptr)
        {
            // TODO: Review the mallocs? And Refactor to a common function
            auto result = (MemoryArena*)malloc(sizeof(MemoryArena)); // TODO: System call to review
            result->MemoryArenaStorage = stackMemoryArenaExtraStorage;
            result->StartOffset = stackMemoryArenaExtraStorage->AllocatedBytes;
            result->ExtraStorage = nullptr;

            memoryArena->ExtraStorage = result;
        }

        workingMemoryArena = memoryArena->ExtraStorage;
    }

    auto storage = workingMemoryArena->MemoryArenaStorage;

    if (storage->AllocatedBytes + sizeInBytes > storage->Memory.Length)
    {
        printf("SIZE Problem!\n");

        auto newSize = storage->Memory.Length * 2;
        auto newMemory = Span<uint8_t>((uint8_t*)malloc(newSize), newSize); // TODO: System call to review

        storage->Memory.CopyTo(newMemory);
        free(storage->Memory.Pointer);
        storage->Memory = newMemory;
    }

    auto result = storage->Memory.Pointer + storage->AllocatedBytes;
    storage->AllocatedBytes += sizeInBytes;

    return result;
}

void* SystemPushMemoryZero(MemoryArena* memoryArena, size_t sizeInBytes)
{
    auto result = SystemPushMemory(memoryArena, sizeInBytes);
    memset(result, 0, sizeInBytes); // TODO: System call to review

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

void SystemPopMemory(MemoryArena* memoryArena, size_t sizeInBytes)
{
    memoryArena->MemoryArenaStorage->AllocatedBytes = memoryArena->MemoryArenaStorage->AllocatedBytes - sizeInBytes;
}

template<typename T>
Span<T> SystemConcatBuffers(MemoryArena* memoryArena, ReadOnlySpan<T> buffer1, ReadOnlySpan<T> buffer2)
{
    auto result = SystemPushArray<T>(memoryArena, buffer1.Length + buffer2.Length);

    buffer1.CopyTo(result);
    buffer2.CopyTo(result.Slice(buffer1.Length));

    return result;
}

template<>
Span<char> SystemConcatBuffers(MemoryArena* memoryArena, ReadOnlySpan<char> buffer1, ReadOnlySpan<char> buffer2)
{
    auto result = SystemPushArrayZero<char>(memoryArena, buffer1.Length + buffer2.Length);

    buffer1.CopyTo(result);
    buffer2.CopyTo(result.Slice(buffer1.Length));

    return result;
}

template<>
Span<wchar_t> SystemConcatBuffers(MemoryArena* memoryArena, ReadOnlySpan<wchar_t> buffer1, ReadOnlySpan<wchar_t> buffer2)
{
    auto result = SystemPushArrayZero<wchar_t>(memoryArena, buffer1.Length + buffer2.Length);

    buffer1.CopyTo(result);
    buffer2.CopyTo(result.Slice(buffer1.Length));

    return result;
}
