#include "SystemMemory.h"

MemoryArena* SystemMemoryArenaAllocate(size_t sizeInBytes)
{
    // TODO: Review the 2 mallocs?
    auto pointer = (uint8_t*)malloc(sizeInBytes); // TODO: System call to review

    auto result = (MemoryArena*)malloc(sizeof(MemoryArena)); // TODO: System call to review
    result->Memory = Span<uint8_t>(pointer, sizeInBytes);
    result->AllocatedBytes = 0;

    return result;
}

void SystemMemoryArenaFree(MemoryArena* memoryArena)
{
    free(memoryArena->Memory.Pointer); // TODO: System call to review
    free(memoryArena); // TODO: System call to review
}

void SystemMemoryArenaClear(MemoryArena* memoryArena)
{
    memoryArena->AllocatedBytes = 0;
}

void* SystemPushMemory(MemoryArena* memoryArena, size_t sizeInBytes)
{
    // TODO: Add checks
    // TODO: Grows the arena if needed

    auto result = memoryArena->Memory.Pointer + memoryArena->AllocatedBytes;
    memoryArena->AllocatedBytes += sizeInBytes;

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
Span<char> SystemPushArray<char>(MemoryArena* memoryArena, size_t count)
{
    auto memory = SystemPushMemoryZero(memoryArena, sizeof(char) * (count + 1));
    return Span<char>((char*)memory, count);
}

template<>
Span<wchar_t> SystemPushArray<wchar_t>(MemoryArena* memoryArena, size_t count)
{
    auto memory = SystemPushMemoryZero(memoryArena, sizeof(wchar_t) * (count + 1));
    return Span<wchar_t>((wchar_t*)memory, count);
}

template<typename T>
T SystemPushStruct(MemoryArena* memoryArena)
{
    return SystemPushMemory(memoryArena, sizeof(T));
}

template<typename T>
T SystemPushStructZero(MemoryArena* memoryArena)
{
    return SystemPushMemoryZero(memoryArena, sizeof(T));
}

void SystemPopMemory(MemoryArena* memoryArena, size_t sizeInBytes)
{
    memoryArena->AllocatedBytes = memoryArena->AllocatedBytes - sizeInBytes;
}

template<typename T>
Span<T> SystemConcatBuffers(MemoryArena* memoryArena, ReadOnlySpan<T> buffer1, ReadOnlySpan<T> buffer2)
{
    auto result = SystemPushArray<T>(memoryArena, buffer1.Length + buffer2.Length);

    buffer1.CopyTo(result);
    buffer2.CopyTo(result.Slice(buffer1.Length));

    return result;
}
