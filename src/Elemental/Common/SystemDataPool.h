#pragma once

#include "Elemental.h"
#include "SystemMemory.h"

template<typename T>
struct SystemDataPoolStorage;

template<typename T>
struct SystemDataPool
{
    SystemDataPoolStorage<T>* Storage;
};

template<typename T>
SystemDataPool<T> SystemCreateDataPool(MemoryArena memoryArena, size_t maxItems);

template<typename T>
ElemHandle SystemAddDataPoolItem(SystemDataPool<T> dataPool, T data);
    
template<typename T>
void SystemRemoveDataPoolItem(SystemDataPool<T> dataPool, ElemHandle handle);

template<typename T>
T* SystemGetDataPoolItem(SystemDataPool<T> dataPool, ElemHandle handle);
