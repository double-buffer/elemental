#pragma once

#include "Elemental.h"
#include "VulkanResource.h"
#include "Graphics/CommandAllocatorPool.h"
#include "Graphics/ResourceBarrier.h"
#include "Graphics/UploadBufferPool.h"
#include "SystemSpan.h"
#include "volk.h"

enum VulkanPipelineStateType
{
    VulkanPipelineStateType_Graphics,
    VulkanPipelineStateType_Compute
};

// TODO: Review data
struct VulkanCommandQueueData
{
    VkQueue DeviceObject;
    uint32_t QueueFamilyIndex;
    CommandAllocatorQueueType CommandAllocatorQueueType;
    ElemGraphicsDevice GraphicsDevice;
    VkSemaphore Fence;
    uint64_t FenceValue;
    VkSemaphore PresentSemaphore;
    bool SignalPresentSemaphore;
    uint64_t LastCompletedFenceValue;
};

struct VulkanCommandQueueDataFull
{
    Span<VkCommandPool> CommandAllocators;
    uint32_t CurrentCommandAllocatorIndex;
    Span<VkCommandBuffer> CommandLists;
    uint32_t CurrentCommandListIndex;
};

struct VulkanCommandListData
{
    VkCommandBuffer DeviceObject;
    ElemGraphicsDevice GraphicsDevice;
    ElemCommandQueue CommandQueue;
    bool IsCommitted;
    CommandAllocatorPoolItem<VkCommandPool, VkCommandBuffer>* CommandAllocatorPoolItem;
    CommandListPoolItem<VkCommandBuffer>* CommandListPoolItem;
    VulkanPipelineStateType PipelineStateType;
    ResourceBarrierPool ResourceBarrierPool;
    UploadBufferPoolItem<VulkanUploadBuffer>* UploadBufferPoolItems[MAX_UPLOAD_BUFFERS];
    uint32_t UploadBufferCount;
};

struct VulkanCommandListDataFull
{
    ElemBeginRenderPassParameters CurrentRenderPassParameters;
};

VulkanCommandQueueData* GetVulkanCommandQueueData(ElemCommandQueue commandQueue);
VulkanCommandQueueDataFull* GetVulkanCommandQueueDataFull(ElemCommandQueue commandQueue);
VulkanCommandListData* GetVulkanCommandListData(ElemCommandList commandList);
VulkanCommandListDataFull* GetVulkanCommandListDataFull(ElemCommandList commandList);

ElemFence CreateVulkanCommandQueueFence(ElemCommandQueue commandQueue);

ElemCommandQueue VulkanCreateCommandQueue(ElemGraphicsDevice graphicsDevice, ElemCommandQueueType type, const ElemCommandQueueOptions* options);
void VulkanFreeCommandQueue(ElemCommandQueue commandQueue);
void VulkanResetCommandAllocation(ElemGraphicsDevice graphicsDevice);
ElemCommandList VulkanGetCommandList(ElemCommandQueue commandQueue, const ElemCommandListOptions* options);
void VulkanCommitCommandList(ElemCommandList commandList);

ElemFence VulkanExecuteCommandLists(ElemCommandQueue commandQueue, ElemCommandListSpan commandLists, const ElemExecuteCommandListOptions* options);
void VulkanWaitForFenceOnCpu(ElemFence fence);
bool VulkanIsFenceCompleted(ElemFence fence);
