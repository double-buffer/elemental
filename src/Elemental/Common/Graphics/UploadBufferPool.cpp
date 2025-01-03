#include "UploadBufferPool.h"
#include "SystemFunctions.h"

template<typename T>
UploadBufferMemory<T> GetUploadBufferPoolItem(UploadBufferDevicePool<T>* uploadBufferPool, uint64_t generation, uint64_t sizeInBytes)
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

        auto currentUploadBuffer = uploadBufferPool->CurrentUploadBuffer;
        uploadBufferPool->CurrentUploadBufferIndex = uploadBufferPool->CurrentUploadBufferIndex + 1 % MAX_UPLOAD_BUFFERS;

        // TODO: Split that into a NeedBufferCreation and NeedBufferDelete
        currentUploadBuffer->IsResetNeeded = true;
        currentUploadBuffer->CurrentOffset = 0u;

        uint64_t alignedSize = SystemMin(uploadBufferPool->LastBufferSize * 2u, 512llu * 1024u * 1024u);

        if (alignedSize == 0)
        {
            // TODO: put that in a constant
            alignedSize = 16u * 1024u * 1024u;
        }

        while (alignedSize < sizeInBytes)
        {
            alignedSize <<= 1;
        }

        uploadBufferPool->LastBufferSize = alignedSize;
        currentUploadBuffer->SizeInBytes = alignedSize;
    }

    // TODO: Check alignment
    auto offset = uploadBufferPool->CurrentUploadBuffer->CurrentOffset;
    uploadBufferPool->CurrentUploadBuffer->CurrentOffset += sizeInBytes;

    return 
    {
        .PoolItem = uploadBufferPool->CurrentUploadBuffer,
        .Offset = offset
    };
}

/*
template<typename TCommandAllocator, typename TCommandList>
CommandListPoolItem<TCommandList>* GetCommandListPoolItem(CommandAllocatorPoolItem<TCommandAllocator, TCommandList>* commandAllocatorPoolItem)
{
    auto commandListPoolItem = &commandAllocatorPoolItem->CommandListPoolItems[commandAllocatorPoolItem->CurrentCommandListIndex];
    SystemAssert (!commandListPoolItem->IsInUse);

    commandAllocatorPoolItem->CurrentCommandListIndex = (commandAllocatorPoolItem->CurrentCommandListIndex + 1) % MAX_COMMANDLIST;

    commandListPoolItem->IsInUse = true;
    return commandListPoolItem;
}

template<typename TCommandList>
void ReleaseCommandListPoolItem(CommandListPoolItem<TCommandList>* commandListPoolItem)
{
    commandListPoolItem->IsInUse = false;
}

template<typename TCommandAllocator, typename TCommandList>
void UpdateCommandAllocatorPoolItemFence(CommandAllocatorPoolItem<TCommandAllocator, TCommandList>* commandAllocatorPoolItem, ElemFence fence)
{
    commandAllocatorPoolItem->Fence = fence;
}*/
