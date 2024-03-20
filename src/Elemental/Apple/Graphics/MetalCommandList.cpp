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

void MetalResetCommandEncoder(ElemCommandList commandList)
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

ElemCommandList MetalCreateCommandList(ElemCommandQueue commandQueue, const ElemCommandListOptions* options)
{
    SystemAssert(commandQueue != ELEM_HANDLE_NULL);

    auto commandQueueData = GetMetalCommandQueueData(commandQueue);
    SystemAssert(commandQueueData);
    
    auto metalCommandBuffer = NS::TransferPtr(commandQueueData->DeviceObject->commandBufferWithUnretainedReferences());
    SystemAssertReturnNullHandle(metalCommandBuffer);

    if (MetalDebugLayerEnabled && options && options->DebugName)
    {
        metalCommandBuffer->setLabel(NS::String::string(options->DebugName, NS::UTF8StringEncoding));
    }
    
    auto handle = SystemAddDataPoolItem(metalCommandListPool, {
        .CommandEncoderType = MetalCommandEncoderType_None
    }); 

    SystemAddDataPoolItemFull(metalCommandListPool, handle, {
        .IsCommitted = false,
        .DeviceObject = metalCommandBuffer
    });

    return handle;
}

void MetalCommitCommandList(ElemCommandList commandList)
{
    MetalResetCommandEncoder(commandList);

    auto commandListDataFull = GetMetalCommandListDataFull(commandList);
    SystemAssert(commandListDataFull);
    commandListDataFull->IsCommitted = true;
}

ElemFence MetalExecuteCommandLists(ElemCommandQueue commandQueue, ElemCommandListSpan commandLists, const ElemExecuteCommandListOptions* options)
{
    return ELEM_HANDLE_NULL;
}

void MetalWaitForFenceOnCpu(ElemFence fence)
{
}
