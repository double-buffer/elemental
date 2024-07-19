#include "MetalResourceBarrier.h"
#include "MetalGraphicsDevice.h"
#include "MetalResource.h"
#include "MetalCommandList.h"
#include "SystemFunctions.h"

// TODO: In metal we will need to have 2 functions this one that is executed like in DX12 before executing the actual stage
// to wait for previous barriers and another on to signal the after parts.
// For the moment the signal part is done in the reset command encoder which is good because we call it at command list commit
// Or end render pass (or begin if we didn't commit and we need to switch to reset it)
// So we can use the info computed by the common code to insert the correct waits. The question is that for the moment we have
// a resource fence that is shared to the whole command queue. Do we need one per resource?

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

    auto shouldWait = (commandQueueData->ResourceBarrierTypes & MetalResourceBarrierType_Fence) != 0;

    // TODO: Do a resource barrier if we still are inside the same encoder (Fence is false)

    if (commandListData->CommandEncoderType == MetalCommandEncoderType_Render)
    {
        auto renderCommandEncoder = (MTL::RenderCommandEncoder*)commandListData->CommandEncoder.get();
        
        if (shouldWait)
        {
            // TODO: Use the proper stage
            renderCommandEncoder->waitForFence(commandQueueData->ResourceFence.get(), MTL::RenderStageFragment);

            // BUG: It seems that waiting for mesh stage is not working???
            //renderCommandEncoder->waitForFence(commandQueueData->ResourceFence.get(), MTL::RenderStageObject);
            //renderCommandEncoder->waitForFence(commandQueueData->ResourceFence.get(), MTL::RenderStageMesh);
        }
    }
    else if (commandListData->CommandEncoderType == MetalCommandEncoderType_Compute)
    {
        auto computeCommandEncoder = (MTL::ComputeCommandEncoder*)commandListData->CommandEncoder.get();
        //renderCommandEncoder->setBytes(data.Items, data.Length, 2);

        if (shouldWait)
        {
            // TODO: Use the proper stage
            computeCommandEncoder->waitForFence(commandQueueData->ResourceFence.get());
        }
    }

    commandQueueData->ResourceBarrierTypes = 0;
}

void MetalGraphicsResourceBarrier(ElemCommandList commandList, ElemGraphicsResourceDescriptor descriptor, const ElemGraphicsResourceBarrierOptions* options)
{
    SystemAssert(commandList != ELEM_HANDLE_NULL);

    auto commandListData = GetMetalCommandListData(commandList);
    SystemAssert(commandListData);

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

    // TODO: Test code!!!
    //commandQueueData->ResourceBarrierTypes |= MetalResourceBarrierType_Texture;
}
