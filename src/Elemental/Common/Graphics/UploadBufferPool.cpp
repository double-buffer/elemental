#include "UploadBufferPool.h"
#include "SystemFunctions.h"

template<typename T>
UploadBufferMemory<T> GetUploadBufferPoolItem(UploadBufferDevicePool<T>* uploadBufferPool, uint64_t generation, uint64_t alignment, uint64_t sizeInBytes)
{
    if (uploadBufferPool->Generation != generation)
    {
        uploadBufferPool->CurrentUploadBuffer = nullptr;
        uploadBufferPool->Generation = generation;
    }

    if (uploadBufferPool->CurrentUploadBuffer != nullptr)
    {
        if (uploadBufferPool->CurrentUploadBuffer->CurrentOffset + sizeInBytes > uploadBufferPool->CurrentUploadBuffer->SizeInBytes)
        {
            uploadBufferPool->CurrentUploadBuffer = nullptr;
            SystemLogDebugMessage(ElemLogMessageCategory_Graphics, "Too big");
        }
    }

    if (uploadBufferPool->CurrentUploadBuffer == nullptr)
    {
        uploadBufferPool->CurrentUploadBuffer = &uploadBufferPool->UploadBuffers[uploadBufferPool->CurrentUploadBufferIndex];
        uploadBufferPool->CurrentUploadBufferIndex = (uploadBufferPool->CurrentUploadBufferIndex + 1) % MAX_UPLOAD_BUFFERS;

        auto currentUploadBuffer = uploadBufferPool->CurrentUploadBuffer;
        uint64_t uploadBufferSize = SystemMin(uploadBufferPool->LastBufferSize * 2u, 512llu * 1024u * 1024u);

        if (uploadBufferSize == 0)
        {
            // TODO: put that in a constant
            uploadBufferSize = 16u * 1024u * 1024u;
        }

        while (uploadBufferSize < sizeInBytes)
        {
            uploadBufferSize <<= 1;
        }

        uploadBufferPool->LastBufferSize = uploadBufferSize;

        if (currentUploadBuffer->SizeInBytes != uploadBufferSize)
        {
            currentUploadBuffer->SizeInBytes = uploadBufferSize;
            currentUploadBuffer->IsResetNeeded = true;
        }

        currentUploadBuffer->CurrentOffset = 0u;
    }

    auto offset = uploadBufferPool->CurrentUploadBuffer->CurrentOffset;
    auto alignedOffset = SystemAlign(offset, alignment);

    uint64_t newOffset = alignedOffset + sizeInBytes;

    // TODO: Better handle that case
    if (newOffset > uploadBufferPool->CurrentUploadBuffer->SizeInBytes)
    {
        SystemLogDebugMessage(ElemLogMessageCategory_Graphics, "Not enough space after alignment");
        return { nullptr, 0ull };
    }

    uploadBufferPool->CurrentUploadBuffer->CurrentOffset = newOffset;
    uploadBufferPool->CurrentUploadBuffer->LastUsedGeneration = generation;

    return 
    {
        .PoolItem = uploadBufferPool->CurrentUploadBuffer,
        .Offset = alignedOffset
    };
}

template<typename T>
void UpdateUploadBufferPoolItemFence(UploadBufferPoolItem<T>* uploadBufferPoolItem, ElemFence fence)
{
    uploadBufferPoolItem->Fence = fence;
}

template<typename T>
Span<UploadBufferPoolItem<T>*> GetUploadBufferPoolItemsToDelete(MemoryArena memoryArena, UploadBufferDevicePool<T>* uploadBufferPool, uint64_t generation)
{
    auto result = SystemPushArray<UploadBufferPoolItem<T>*>(memoryArena, MAX_UPLOAD_BUFFERS);
    auto resultCount = 0u;

    for (uint32_t i = 0; i < MAX_UPLOAD_BUFFERS; i++)
    {
        auto uploadBufferPoolItem = &uploadBufferPool->UploadBuffers[i];

        if (uploadBufferPoolItem->SizeInBytes > 0 && (generation - uploadBufferPoolItem->LastUsedGeneration > 500))
        {
            result[resultCount++] = uploadBufferPoolItem;
        }
    }

    return result.Slice(0, resultCount);
}
