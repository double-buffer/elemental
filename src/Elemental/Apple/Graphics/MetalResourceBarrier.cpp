#include "MetalResourceBarrier.h"
#include "MetalGraphicsDevice.h"
#include "MetalResource.h"
#include "MetalCommandList.h"
#include "SystemFunctions.h"

void InsertMetalResourceBarriersIfNeeded(ElemCommandList commandList, ResourceBarrierSyncType currentStage)
{
    auto stackMemoryArena = SystemGetStackMemoryArena();

    SystemAssert(commandList != ELEM_HANDLE_NULL);

    auto commandListData = GetMetalCommandListData(commandList);
    SystemAssert(commandListData);

    auto commandQueueData = GetMetalCommandQueueData(commandListData->CommandQueue);
    SystemAssert(commandQueueData);
    
    auto barriersInfo = GenerateBarrierCommands(stackMemoryArena, commandListData->ResourceBarrierPool, currentStage, MetalDebugBarrierInfoEnabled);

    if (barriersInfo.BufferBarriers.Length == 0 && barriersInfo.TextureBarriers.Length == 0)
    {
        return;
    }
    
    auto totalResourceCount = barriersInfo.BufferBarriers.Length + barriersInfo.TextureBarriers.Length;
    auto barrierResourceList = SystemPushArray<MTL::Resource*>(stackMemoryArena, totalResourceCount);
    auto barrierCount = 0u;

    for (uint32_t i = 0; i < barriersInfo.BufferBarriers.Length; i++)
    {
        auto bufferBarrier = barriersInfo.BufferBarriers[i];
        auto resourceData = GetMetalResourceData(bufferBarrier.Resource);
        SystemAssert(resourceData);

        barrierResourceList[barrierCount++] = resourceData->DeviceObject.get();
    }
    
    for (uint32_t i = 0; i < barriersInfo.TextureBarriers.Length; i++)
    {
        auto textureBarrier = barriersInfo.TextureBarriers[i];
        auto resourceData = GetMetalResourceData(textureBarrier.Resource);
        SystemAssert(resourceData);

        barrierResourceList[barrierCount++] = resourceData->DeviceObject.get();
    }

    auto shouldWait = (commandQueueData->ResourceBarrierTypes & MetalResourceBarrierType_Fence) != 0;

    if (commandListData->CommandEncoderType == MetalCommandEncoderType_Render)
    {
        auto renderCommandEncoder = (MTL::RenderCommandEncoder*)commandListData->CommandEncoder.get();
        
        if (shouldWait)
        {
            // TODO: Use the proper stage
            renderCommandEncoder->waitForFence(commandQueueData->ResourceFence.get(), MTL::RenderStageVertex);

            // BUG: It seems that waiting for mesh stage is not working???
            //renderCommandEncoder->waitForFence(commandQueueData->ResourceFence.get(), MTL::RenderStageObject);
            //renderCommandEncoder->waitForFence(commandQueueData->ResourceFence.get(), MTL::RenderStageMesh);
        }
        else
        {
            // TODO: Use the proper stage
            renderCommandEncoder->memoryBarrier(barrierResourceList.Pointer, barrierResourceList.Length, MTL::RenderStageFragment, MTL::RenderStageVertex);
        }
    }
    else if (commandListData->CommandEncoderType == MetalCommandEncoderType_Compute)
    {
        auto computeCommandEncoder = (MTL::ComputeCommandEncoder*)commandListData->CommandEncoder.get();

        if (shouldWait)
        {
            computeCommandEncoder->waitForFence(commandQueueData->ResourceFence.get());
        }
        else 
        {
            computeCommandEncoder->memoryBarrier(barrierResourceList.Pointer, barrierResourceList.Length);
        }
    }

    commandQueueData->ResourceBarrierTypes = 0;
}

void MetalGraphicsResourceBarrier(ElemCommandList commandList, ElemGraphicsResourceDescriptor descriptor, const ElemGraphicsResourceBarrierOptions* options)
{
    SystemAssert(commandList != ELEM_HANDLE_NULL);

    auto commandListData = GetMetalCommandListData(commandList);
    SystemAssert(commandListData);
    
    auto commandQueueData = GetMetalCommandQueueData(commandListData->CommandQueue);
    SystemAssert(commandQueueData);

    auto descriptorInfo = MetalGetGraphicsResourceDescriptorInfo(descriptor);
    auto resourceInfo = MetalGetGraphicsResourceInfo(descriptorInfo.Resource);
    
    ResourceBarrierItem resourceBarrier =
    {
        .Type = resourceInfo.Type,
        .Resource = descriptorInfo.Resource,
        .AccessAfter = (descriptorInfo.Usage & ElemGraphicsResourceDescriptorUsage_Write) ? AccessType_Write : AccessType_Read,
        .LayoutAfter = (descriptorInfo.Usage & ElemGraphicsResourceDescriptorUsage_Write) ? LayoutType_Write : LayoutType_Read
        // TODO: Options
    };

    EnqueueBarrier(commandListData->ResourceBarrierPool, &resourceBarrier);

    // TODO: We can simplify that because we handle memory barrier differently
    if (resourceInfo.Type == ElemGraphicsResourceType_Buffer)
    {
        commandQueueData->ResourceBarrierTypes |= MetalResourceBarrierType_Buffer;
    }
    else
    {
        commandQueueData->ResourceBarrierTypes |= MetalResourceBarrierType_Texture;
    }
}
