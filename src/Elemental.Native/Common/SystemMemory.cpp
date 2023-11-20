#include "SystemMemory.h"

MemoryArena* SystemAllocateMemoryArena(size_t sizeInBytes)
{
    // TODO: Review the 2 mallocs?
    auto pointer = (uint8_t*)malloc(sizeInBytes);

    auto result = (MemoryArena*)malloc(sizeof(MemoryArena));
    result->Memory = Span<uint8_t>(pointer, sizeInBytes);
    result->AllocatedBytes = 0;

    return result;
}

void SystemFreeMemoryArena(MemoryArena* memoryArena)
{
    free(memoryArena->Memory.Pointer);
    free(memoryArena);
}

void* SystemAllocateMemory(MemoryArena* memoryArena, size_t sizeInBytes)
{
    // TODO: Add checks

    auto result = memoryArena->Memory.Pointer + memoryArena->AllocatedBytes;
    memoryArena->AllocatedBytes += sizeInBytes;

    // TODO: Make it optional
    memset(result, 0, sizeInBytes);

    return result;
}

template<typename T>
Span<T> SystemPushArray(MemoryArena* memoryArena, size_t count)
{
    auto memory = SystemAllocateMemory(memoryArena, sizeof(T) * count);
    return Span<T>((T*)memory, count);
}

template<>
Span<char> SystemPushArray<char>(MemoryArena* memoryArena, size_t count)
{
    auto memory = SystemAllocateMemory(memoryArena, sizeof(char) * (count + 1));
    return Span<char>((char*)memory, count);
}

template<>
Span<wchar_t> SystemPushArray<wchar_t>(MemoryArena* memoryArena, size_t count)
{
    auto memory = SystemAllocateMemory(memoryArena, sizeof(wchar_t) * (count + 1));
    return Span<wchar_t>((wchar_t*)memory, count);
}

template<typename T>
Span<T> SystemConcatBuffers(MemoryArena* memoryArena, ReadOnlySpan<T> buffer1, ReadOnlySpan<T> buffer2)
{
    auto result = SystemPushArray<T>(memoryArena, buffer1.Length + buffer2.Length);

    buffer1.CopyTo(result);
    buffer2.CopyTo(result.Slice(buffer1.Length));

    return result;
}
