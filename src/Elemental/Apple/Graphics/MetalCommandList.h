#pragma once

#include "Elemental.h"
#include "Graphics/ResourceBarrier.h"

enum MetalCommandEncoderType
{
    MetalCommandEncoderType_None = 0,
    MetalCommandEncoderType_Render = 1,
    MetalCommandEncoderType_Compute = 2,
    MetalCommandEncoderType_Copy = 3
};

enum MetalResourceBarrierType
{
    MetalResourceBarrierType_None = 0,
    MetalResourceBarrierType_Buffer = 1,
    MetalResourceBarrierType_Texture = 2,
    MetalResourceBarrierType_RenderTarget = 4,
    MetalResourceBarrierType_Fence = 8,
};

struct MetalCommandQueueData
{
    NS::SharedPtr<MTL::CommandQueue> DeviceObject;
    NS::SharedPtr<MTL::Fence> ResourceFence;
    NS::SharedPtr<MTL::SharedEvent> QueueEvent;
    uint64_t FenceValue;
    uint32_t ResourceBarrierTypes;
    ElemGraphicsDevice GraphicsDevice;
};

struct MetalCommandQueueDataFull
{
    uint64_t LastCompletedFenceValue;
};

struct MetalCommandListData
{
    NS::SharedPtr<MTL::CommandBuffer> DeviceObject;
    NS::SharedPtr<MTL::CommandEncoder> CommandEncoder;
    ElemGraphicsDevice GraphicsDevice;
    ElemCommandQueue CommandQueue;
    MetalCommandEncoderType CommandEncoderType;
    ElemPipelineState PipelineState;
    bool IsCommitted;
    ResourceBarrierPool ResourceBarrierPool;
    bool ArgumentBufferBound;
    UploadBufferPoolItem<NS::SharedPtr<MTL::Buffer>>* UploadBufferPoolItems[MAX_UPLOAD_BUFFERS];
    uint32_t UploadBufferCount;
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
bool MetalIsFenceCompleted(ElemFence fence);
