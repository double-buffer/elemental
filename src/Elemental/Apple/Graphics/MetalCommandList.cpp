#include "MetalCommandList.h"
#include "MetalGraphicsDevice.h"
#include "SystemDataPool.h"
#include "SystemFunctions.h"
#include "SystemMemory.h"

#define METAL_MAX_COMMANDQUEUES 10u
#define METAL_MAX_COMMANDLISTS 64u

SystemDataPool<MetalCommandQueueData, MetalCommandQueueDataFull> metalCommandQueuePool;
SystemDataPool<MetalCommandListData, MetalCommandListDataFull> metalCommandListPool;

void InitMetalCommandListMemory()
{
    if (!metalCommandQueuePool.Storage)
    {
        metalCommandQueuePool = SystemCreateDataPool<MetalCommandQueueData, MetalCommandQueueDataFull>(MetalGraphicsMemoryArena, METAL_MAX_COMMANDQUEUES);
        metalCommandListPool = SystemCreateDataPool<MetalCommandListData, MetalCommandListDataFull>(MetalGraphicsMemoryArena, METAL_MAX_COMMANDLISTS);
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

    if (commandListData->CommandEncoder)
    {
        commandListData->CommandEncoder->endEncoding();
        commandListData->CommandEncoder.reset();
        commandListData->CommandEncoderType = MetalCommandEncoderType_None;
    }
}

ElemCommandQueue MetalCreateCommandQueue(ElemGraphicsDevice graphicsDevice, ElemCommandQueueType type, const ElemCommandQueueOptions* options)
{
    InitMetalCommandListMemory();
    SystemAssert(graphicsDevice != ELEM_HANDLE_NULL);
    
    auto graphicsDeviceData = GetMetalGraphicsDeviceData(graphicsDevice);
    SystemAssert(graphicsDeviceData);

    auto metalCommandQueue = NS::TransferPtr(graphicsDeviceData->Device->newCommandQueue(METAL_MAX_COMMANDLISTS));
    SystemAssertReturnNullHandle(metalCommandQueue);

    if (MetalDebugLayerEnabled && options && options->DebugName)
    {
        metalCommandQueue->setLabel(NS::String::string(options->DebugName, NS::UTF8StringEncoding));
    }
    
    auto handle = SystemAddDataPoolItem(metalCommandQueuePool, {
        .DeviceObject = metalCommandQueue
    }); 

    SystemAddDataPoolItemFull(metalCommandQueuePool, handle, {
        .GraphicsDevice = graphicsDevice
    });

    return handle;
}

void MetalFreeCommandQueue(ElemCommandQueue commandQueue)
{
    SystemAssert(commandQueue != ELEM_HANDLE_NULL);

    auto commandQueueData = GetMetalCommandQueueData(commandQueue);
    SystemAssert(commandQueueData);

    // TODO: Wait for command queue fence?

    commandQueueData->DeviceObject.reset();
}

void MetalResetCommandAllocation(ElemGraphicsDevice graphicsDevice)
{
}

ElemCommandList MetalGetCommandList(ElemCommandQueue commandQueue, const ElemCommandListOptions* options)
{
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
        .CommandEncoderType = MetalCommandEncoderType_None,
        .IsCommitted = false
    }); 

    SystemAddDataPoolItemFull(metalCommandListPool, handle, {
    });

    return handle;
}

void MetalCommitCommandList(ElemCommandList commandList)
{
    ResetMetalCommandEncoder(commandList);

    auto commandListData = GetMetalCommandListData(commandList);
    SystemAssert(commandListData);
    commandListData->IsCommitted = true;
}

ElemFence MetalExecuteCommandLists(ElemCommandQueue commandQueue, ElemCommandListSpan commandLists, const ElemExecuteCommandListOptions* options)
{
    // TODO: This is really bad because we should reuse the command lists objects
    for (uint32_t i = 0; i < commandLists.Length; i++)
    {
        auto commandListData = GetMetalCommandListData(commandLists.Items[i]);
        SystemAssert(commandListData);

        commandListData->DeviceObject->commit();
        commandListData->DeviceObject.reset();

        SystemRemoveDataPoolItem(metalCommandListPool, commandLists.Items[i]);
    }
    return {};
}

void MetalWaitForFenceOnCpu(ElemFence fence)
{
}
