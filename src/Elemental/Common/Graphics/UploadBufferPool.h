#pragma once

#include "Elemental.h"
#include "SystemMemory.h"

#define MAX_UPLOAD_BUFFERS 10

template<typename T>
struct UploadBufferPoolItem
{
    T Buffer;
    uint64_t SizeInBytes;
    ElemFence Fence;
    bool IsResetNeeded;
    uint64_t CurrentOffset;
    uint8_t* CpuPointer;
    uint64_t LastUsedGeneration;
};

template<typename T>
struct UploadBufferMemory
{
    UploadBufferPoolItem<T>* PoolItem;
    uint64_t Offset;
};

template<typename T>
struct UploadBufferDevicePool
{
    UploadBufferPoolItem<T> UploadBuffers[MAX_UPLOAD_BUFFERS]; 
    uint32_t CurrentUploadBufferIndex;
    UploadBufferPoolItem<T>* CurrentUploadBuffer;
    uint64_t LastBufferSize;
    uint64_t Generation;
    bool IsInited;
};

template<typename T>
UploadBufferMemory<T> GetUploadBufferPoolItem(UploadBufferDevicePool<T>* uploadBufferPool, uint64_t generation, uint64_t alignment, uint64_t sizeInBytes);

template<typename T>
void UpdateUploadBufferPoolItemFence(UploadBufferPoolItem<T>* uploadBufferPoolItem, ElemFence fence);

template<typename T>
Span<UploadBufferPoolItem<T>*> GetUploadBufferPoolItemsToDelete(MemoryArena memoryArena, UploadBufferDevicePool<T>* uploadBufferPool, uint64_t generation);
