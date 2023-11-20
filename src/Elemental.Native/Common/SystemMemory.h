#pragma once

#include "SystemSpan.h"

//#define new NOT_IMPLEMENTED()

//#define malloc(size) NOT_IMPLEMENTED(size)
//#define calloc(count, size) NOT_IMPLEMENTED(count, size)
//#define free(pointer) NOT_IMPLEMENTED(pointer)

struct MemoryArena
{
    Span<uint8_t> Memory;
    size_t AllocatedBytes;
};

MemoryArena* SystemAllocateMemoryArena(size_t sizeInBytes);
void SystemFreeMemoryArena(MemoryArena* memoryArena);

template<typename T>
Span<T> SystemPushArray(MemoryArena* memoryArena, size_t count); 

template<typename T>
Span<T> SystemConcatBuffers(MemoryArena* memoryArena, ReadOnlySpan<T> buffer1, ReadOnlySpan<T> buffer2);
