#pragma once

#include "SystemSpan.h"

//#define new NOT_IMPLEMENTED()

//#define malloc(size) NOT_IMPLEMENTED(size)
//#define calloc(count, size) NOT_IMPLEMENTED(count, size)
//#define free(pointer) NOT_IMPLEMENTED(pointer)

struct MemoryArenaStorage;

struct MemoryArena
{
    MemoryArenaStorage* MemoryArenaStorage;
    size_t StartOffset;
    uint8_t Level;
    MemoryArena* ExtraStorage;
};

struct StackMemoryArena
{
    MemoryArena* MemoryArenaPointer;

    ~StackMemoryArena();

    operator MemoryArena*() const 
    {
        return MemoryArenaPointer;
    }
};

MemoryArena* SystemAllocateMemoryArena(size_t sizeInBytes);
void SystemFreeMemoryArena(MemoryArena* memoryArena);
void SystemClearMemoryArena(MemoryArena* memoryArena);
size_t SystemGetMemoryArenaAllocatedBytes(MemoryArena* memoryArena);
StackMemoryArena SystemGetStackMemoryArena();

void* SystemPushMemory(MemoryArena* memoryArena, size_t sizeInBytes);
void* SystemPushMemoryZero(MemoryArena* memoryArena, size_t sizeInBytes);
template<typename T>
Span<T> SystemPushArray(MemoryArena* memoryArena, size_t count);
template<typename T>
Span<T> SystemPushArrayZero(MemoryArena* memoryArena, size_t count);
template<typename T>
T* SystemPushStruct(MemoryArena* memoryArena);
template<typename T>
T* SystemPushStructZero(MemoryArena* memoryArena);
void SystemPopMemory(MemoryArena* memoryArena, size_t sizeInBytes);

template<typename T>
Span<T> SystemConcatBuffers(MemoryArena* memoryArena, ReadOnlySpan<T> buffer1, ReadOnlySpan<T> buffer2);
