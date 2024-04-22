#pragma once

#include "Elemental.h"
#include "volk.h"

// TODO: To move
enum CommandAllocatorQueueType
{
    CommandAllocatorQueueType_Graphics = 0,
    CommandAllocatorQueueType_Compute = 1,
    CommandAllocatorQueueType_Copy = 2,
    CommandAllocatorQueueType_Max = 3
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
    uint64_t LastCompletedFenceValue;
};

struct VulkanCommandQueueDataFull
{
    uint32_t reserved;
};

struct VulkanCommandListData
{
    VkCommandBuffer DeviceObject;
};

struct VulkanCommandListDataFull
{
    ElemBeginRenderPassParameters CurrentRenderPassParameters;
};

VulkanCommandQueueData* GetVulkanCommandQueueData(ElemCommandQueue commandQueue);
VulkanCommandQueueDataFull* GetVulkanCommandQueueDataFull(ElemCommandQueue commandQueue);
VulkanCommandListData* GetVulkanCommandListData(ElemCommandList commandList);
VulkanCommandListDataFull* GetVulkanCommandListDataFull(ElemCommandList commandList);

ElemCommandQueue VulkanCreateCommandQueue(ElemGraphicsDevice graphicsDevice, ElemCommandQueueType type, const ElemCommandQueueOptions* options);
void VulkanFreeCommandQueue(ElemCommandQueue commandQueue);
ElemCommandList VulkanGetCommandList(ElemCommandQueue commandQueue, const ElemCommandListOptions* options);
void VulkanCommitCommandList(ElemCommandList commandList);

ElemFence VulkanExecuteCommandLists(ElemCommandQueue commandQueue, ElemCommandListSpan commandLists, const ElemExecuteCommandListOptions* options);
void VulkanWaitForFenceOnCpu(ElemFence fence);
