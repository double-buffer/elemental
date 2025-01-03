#pragma once

#include "Elemental.h"

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
UploadBufferMemory<T> GetUploadBufferPoolItem(UploadBufferDevicePool<T>* uploadBufferPool, uint64_t generation, uint64_t sizeInBytes);

/*
template<typename TCommandList>
void ReleaseCommandListPoolItem(CommandListPoolItem<TCommandList>* commandListPoolItem);

template<typename TCommandAllocator, typename TCommandList>
void UpdateCommandAllocatorPoolItemFence(CommandAllocatorPoolItem<TCommandAllocator, TCommandList>* commandAllocatorPoolItem, ElemFence fence);
*/
