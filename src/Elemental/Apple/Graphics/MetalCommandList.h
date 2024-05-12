#pragma once

#include "Elemental.h"

enum MetalCommandEncoderType
{
    MetalCommandEncoderType_None = 0,
    MetalCommandEncoderType_Render = 1,
    MetalCommandEncoderType_Compute = 2
};

struct MetalCommandQueueData
{
    NS::SharedPtr<MTL::CommandQueue> DeviceObject;
};

struct MetalCommandQueueDataFull
{
    ElemGraphicsDevice GraphicsDevice;
};

struct MetalCommandListData
{
    NS::SharedPtr<MTL::CommandBuffer> DeviceObject;
    NS::SharedPtr<MTL::CommandEncoder> CommandEncoder;
    MetalCommandEncoderType CommandEncoderType;
    bool IsCommitted;
};

struct MetalCommandListDataFull
{
    uint32_t reserved;
};


MetalCommandQueueData* GetMetalCommandQueueData(ElemCommandQueue commandQueue);
MetalCommandQueueDataFull* GetMetalCommandQueueDataFull(ElemCommandQueue commandQueue);
MetalCommandListData* GetMetalCommandListData(ElemCommandList commandList);
MetalCommandListDataFull* GetMetalCommandListDataFull(ElemCommandList commandList);

void ResetMetalCommandEncoder(ElemCommandList commandList);

ElemCommandQueue MetalCreateCommandQueue(ElemGraphicsDevice graphicsDevice, ElemCommandQueueType type, const ElemCommandQueueOptions* options);
void MetalFreeCommandQueue(ElemCommandQueue commandQueue);
void MetalResetCommandAllocation(ElemGraphicsDevice graphicsDevice);
ElemCommandList MetalGetCommandList(ElemCommandQueue commandQueue, const ElemCommandListOptions* options);
void MetalCommitCommandList(ElemCommandList commandList);

ElemFence MetalExecuteCommandLists(ElemCommandQueue commandQueue, ElemCommandListSpan commandLists, const ElemExecuteCommandListOptions* options);
void MetalWaitForFenceOnCpu(ElemFence fence);
