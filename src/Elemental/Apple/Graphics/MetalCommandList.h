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
    uint32_t reverved;
};

struct MetalCommandListData
{
    NS::SharedPtr<MTL::CommandEncoder> CommandEncoder;
    MetalCommandEncoderType CommandEncoderType;
};

struct MetalCommandListDataFull
{
    bool IsCommitted;
    NS::SharedPtr<MTL::CommandBuffer> DeviceObject;
};


MetalCommandQueueData* GetMetalCommandQueueData(ElemCommandQueue commandQueue);
MetalCommandQueueDataFull* GetMetalCommandQueueDataFull(ElemCommandQueue commandQueue);
MetalCommandListData* GetMetalCommandListData(ElemCommandList commandList);
MetalCommandListDataFull* GetMetalCommandListDataFull(ElemCommandList commandList);

ElemCommandQueue MetalCreateCommandQueue(ElemGraphicsDevice graphicsDevice, ElemCommandQueueType type, const ElemCommandQueueOptions* options);
void MetalFreeCommandQueue(ElemCommandQueue commandQueue);
ElemCommandList MetalCreateCommandList(ElemCommandQueue commandQueue, const ElemCommandListOptions* options);
void MetalCommitCommandList(ElemCommandList commandList);

ElemFence MetalExecuteCommandLists(ElemCommandQueue commandQueue, ElemCommandListSpan commandLists, const ElemExecuteCommandListOptions* options);
void MetalWaitForFenceOnCpu(ElemFence fence);
