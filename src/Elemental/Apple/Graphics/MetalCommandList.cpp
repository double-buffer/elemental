#include "MetalCommandList.h"
#include "MetalGraphicsDevice.h"
#include "SystemDataPool.h"
#include "SystemFunctions.h"
#include "SystemMemory.h"

#define METAL_MAX_COMMANDQUEUES 10u
#define METAL_MAX_COMMANDLISTS 64u

SystemDataPool<MetalCommandQueueData, MetalCommandQueueDataFull> metalCommandQueuePool;
SystemDataPool<MetalCommandListData, MetalCommandListDataFull> metalCommandListPool;

MTL::SharedEventListener* globalSharedEventListener;

thread_local bool metalThreadCommandBufferCommitted = true;

void InitMetalCommandListMemory()
{
    if (!metalCommandQueuePool.Storage)
    {
        metalCommandQueuePool = SystemCreateDataPool<MetalCommandQueueData, MetalCommandQueueDataFull>(MetalGraphicsMemoryArena, METAL_MAX_COMMANDQUEUES);
        metalCommandListPool = SystemCreateDataPool<MetalCommandListData, MetalCommandListDataFull>(MetalGraphicsMemoryArena, METAL_MAX_COMMANDLISTS);

        globalSharedEventListener = MTL::SharedEventListener::alloc()->init();
    }
}

MetalCommandQueueData* GetMetalCommandQueueData(ElemCommandQueue commandQueue)
{
    return SystemGetDataPoolItem(metalCommandQueuePool, commandQueue);
}

MetalCommandQueueDataFull* GetMetalCommandQueueDataFull(ElemCommandQueue commandQueue)
{
    return SystemGetDataPoolItemFull(metalCommandQueuePool, commandQueue);
}

MetalCommandListData* GetMetalCommandListData(ElemCommandList commandList)
{
    return SystemGetDataPoolItem(metalCommandListPool, commandList);
}

MetalCommandListDataFull* GetMetalCommandListDataFull(ElemCommandList commandList)
{
    return SystemGetDataPoolItemFull(metalCommandListPool, commandList);
}

void ResetMetalCommandEncoder(ElemCommandList commandList)
{
    auto commandListData = GetMetalCommandListData(commandList);
    SystemAssert(commandListData);

    auto commandQueueData = GetMetalCommandQueueData(commandListData->CommandQueue);
    SystemAssert(commandQueueData);

    if (commandListData->CommandEncoder)
    {
        if (commandQueueData->ResourceBarrierTypes != 0)
        {
            commandQueueData->ResourceBarrierTypes |= MetalResourceBarrierType_Fence;

            if (commandListData->CommandEncoderType == MetalCommandEncoderType_Render)
            {
                auto renderCommandEncoder = (MTL::RenderCommandEncoder*)commandListData->CommandEncoder.get();
                // TODO: Use the proper stage based on sync
                renderCommandEncoder->updateFence(commandQueueData->ResourceFence.get(), MTL::RenderStageFragment);
            }
            else if (commandListData->CommandEncoderType == MetalCommandEncoderType_Compute)
            {
                auto computeCommandEncoder = (MTL::ComputeCommandEncoder*)commandListData->CommandEncoder.get();
                computeCommandEncoder->updateFence(commandQueueData->ResourceFence.get());
            }
        }

        commandListData->CommandEncoder->endEncoding();
        commandListData->CommandEncoder.reset();
        commandListData->CommandEncoderType = MetalCommandEncoderType_None;
    }
}

ElemFence CreateMetalCommandQueueFence(ElemCommandQueue commandQueue, MTL::CommandBuffer* commandBuffer)
{
    auto commandQueueData = GetMetalCommandQueueData(commandQueue);
    SystemAssert(commandQueueData);

    auto commandQueueDataFull = GetMetalCommandQueueDataFull(commandQueue);
    SystemAssert(commandQueueDataFull);

    auto fenceValue = SystemAtomicAdd(commandQueueData->FenceValue, 1) + 1;
    commandBuffer->encodeSignalEvent(commandQueueData->QueueEvent.get(), fenceValue);

    return 
    {
        .CommandQueue = commandQueue,
        .FenceValue = fenceValue
    };
}

ElemCommandQueue MetalCreateCommandQueue(ElemGraphicsDevice graphicsDevice, ElemCommandQueueType type, const ElemCommandQueueOptions* options)
{
    InitMetalCommandListMemory();
    SystemAssert(graphicsDevice != ELEM_HANDLE_NULL);
    
    auto graphicsDeviceData = GetMetalGraphicsDeviceData(graphicsDevice);
    SystemAssert(graphicsDeviceData);

    auto graphicsDeviceDataFull = GetMetalGraphicsDeviceDataFull(graphicsDevice);
    SystemAssert(graphicsDeviceDataFull);

    auto metalCommandQueue = NS::TransferPtr(graphicsDeviceData->Device->newCommandQueue(METAL_MAX_COMMANDLISTS));
    SystemAssertReturnNullHandle(metalCommandQueue);

    if (MetalDebugLayerEnabled && options && options->DebugName)
    {
        metalCommandQueue->setLabel(NS::String::string(options->DebugName, NS::UTF8StringEncoding));
    }

    auto resourceFence = NS::TransferPtr(graphicsDeviceData->Device->newFence());
    auto queueEvent = NS::TransferPtr(graphicsDeviceData->Device->newSharedEvent());

    metalCommandQueue->addResidencySet(graphicsDeviceDataFull->ResidencySet.get());
    
    auto handle = SystemAddDataPoolItem(metalCommandQueuePool, {
        .DeviceObject = metalCommandQueue,
        .ResourceFence = resourceFence,
        .QueueEvent = queueEvent,
        .GraphicsDevice = graphicsDevice
    }); 

    SystemAddDataPoolItemFull(metalCommandQueuePool, handle, {
    });

    return handle;
}

void MetalFreeCommandQueue(ElemCommandQueue commandQueue)
{
    SystemAssert(commandQueue != ELEM_HANDLE_NULL);

    auto commandQueueData = GetMetalCommandQueueData(commandQueue);
    SystemAssert(commandQueueData);

    // TODO: Wait for command queue fence?

    metalThreadCommandBufferCommitted = true;

    //commandQueueData->QueueEvent.reset();
    commandQueueData->DeviceObject.reset();
}

void MetalResetCommandAllocation(ElemGraphicsDevice graphicsDevice)
{
}

ElemCommandList MetalGetCommandList(ElemCommandQueue commandQueue, const ElemCommandListOptions* options)
{
    if (!metalThreadCommandBufferCommitted)
    {
        SystemLogErrorMessage(ElemLogMessageCategory_Graphics, "Cannot get a command list if commit was not called on the same thread.");
        return ELEM_HANDLE_NULL;
    }

    SystemAssert(commandQueue != ELEM_HANDLE_NULL);

    auto commandQueueData = GetMetalCommandQueueData(commandQueue);
    SystemAssert(commandQueueData);
    
    // TODO: We use retain ptr because the command buffer cannot be destroyed while it is executed.
    // Normally it will be released by the auto pool. Confirm that! Normally there is no new in the function name
    // So we don't own it
    auto metalCommandBuffer = NS::RetainPtr(commandQueueData->DeviceObject->commandBufferWithUnretainedReferences());
    SystemAssertReturnNullHandle(metalCommandBuffer);

    if (MetalDebugLayerEnabled && options && options->DebugName)
    {
        metalCommandBuffer->setLabel(NS::String::string(options->DebugName, NS::UTF8StringEncoding));
    }
    
    auto handle = SystemAddDataPoolItem(metalCommandListPool, {
        .DeviceObject = metalCommandBuffer,
        .GraphicsDevice = commandQueueData->GraphicsDevice,
        .CommandQueue = commandQueue,
        .CommandEncoderType = MetalCommandEncoderType_None,
        .IsCommitted = false
    }); 

    SystemAddDataPoolItemFull(metalCommandListPool, handle, {
    });

    metalThreadCommandBufferCommitted = false;

    return handle;
}

void MetalCommitCommandList(ElemCommandList commandList)
{
    ResetMetalCommandEncoder(commandList);

    auto commandListData = GetMetalCommandListData(commandList);
    SystemAssert(commandListData);
    commandListData->IsCommitted = true;
    metalThreadCommandBufferCommitted = true;
}

ElemFence MetalExecuteCommandLists(ElemCommandQueue commandQueue, ElemCommandListSpan commandLists, const ElemExecuteCommandListOptions* options)
{
    SystemAssert(commandQueue != ELEM_HANDLE_NULL);
    
    auto commandQueueData = GetMetalCommandQueueData(commandQueue);
    SystemAssert(commandQueueData);

    ElemFence fence = {};

    // TODO: This is really bad because we should reuse the command lists objects
    for (uint32_t i = 0; i < commandLists.Length; i++)
    {
        auto commandListData = GetMetalCommandListData(commandLists.Items[i]);
        SystemAssert(commandListData);

        if (!commandListData->IsCommitted)
        {
            SystemLogErrorMessage(ElemLogMessageCategory_Graphics, "Commandlist needs to be committed before executing it.");
            return {};
        }

        if (i == commandLists.Length - 1) 
        {
            fence = CreateMetalCommandQueueFence(commandQueue, commandListData->DeviceObject.get());
        }

        commandListData->DeviceObject->commit();
        commandListData->DeviceObject.reset();

        SystemRemoveDataPoolItem(metalCommandListPool, commandLists.Items[i]);
    }

    return fence;
}

void MetalWaitForFenceOnCpu(ElemFence fence)
{
    SystemAssert(fence.CommandQueue != ELEM_HANDLE_NULL);

    auto commandQueueToWaitData = GetMetalCommandQueueData(fence.CommandQueue);
    SystemAssert(commandQueueToWaitData);

    auto commandQueueToWaitDataFull = GetMetalCommandQueueDataFull(fence.CommandQueue);
    SystemAssert(commandQueueToWaitDataFull);

    if (fence.FenceValue > commandQueueToWaitDataFull->LastCompletedFenceValue) 
    {
        commandQueueToWaitDataFull->LastCompletedFenceValue = SystemMax(commandQueueToWaitDataFull->LastCompletedFenceValue, commandQueueToWaitData->QueueEvent->signaledValue());
    }

    if (fence.FenceValue > commandQueueToWaitDataFull->LastCompletedFenceValue)
    {
        SystemLogDebugMessage(ElemLogMessageCategory_Graphics, "Wait for Fence on CPU...");
    
        auto dispatchGroup = dispatch_group_create();
        dispatch_group_enter(dispatchGroup);

        commandQueueToWaitData->QueueEvent->notifyListener(globalSharedEventListener, fence.FenceValue, ^(MTL::SharedEvent* event, uint64_t value)
        {
            dispatch_group_leave(dispatchGroup);
        });

        // TODO: It is a little bit extreme to block at infinity. We should set a timeout and break the program.
        dispatch_group_wait(dispatchGroup, DISPATCH_TIME_FOREVER);
    }
}

bool MetalIsFenceCompleted(ElemFence fence)
{
    SystemAssert(fence.CommandQueue != ELEM_HANDLE_NULL);

    auto commandQueueToWaitData = GetMetalCommandQueueData(fence.CommandQueue);
    SystemAssert(commandQueueToWaitData);

    auto commandQueueToWaitDataFull = GetMetalCommandQueueDataFull(fence.CommandQueue);
    SystemAssert(commandQueueToWaitDataFull);

    if (fence.FenceValue > commandQueueToWaitDataFull->LastCompletedFenceValue) 
    {
        commandQueueToWaitDataFull->LastCompletedFenceValue = SystemMax(commandQueueToWaitDataFull->LastCompletedFenceValue, commandQueueToWaitData->QueueEvent->signaledValue());
    }

    return fence.FenceValue <= commandQueueToWaitDataFull->LastCompletedFenceValue;
}
