#pragma once

#include "Elemental.h"

struct VulkanCommandQueueData
{
};

struct VulkanCommandQueueDataFull
{
};

VulkanCommandQueueData* GetVulkanCommandQueueData(ElemCommandQueue graphicsCommandQueue);
VulkanCommandQueueDataFull* GetVulkanCommandQueueDataFull(ElemCommandQueue graphicsCommandQueue);

ElemCommandQueue VulkanCreateCommandQueue(ElemGraphicsDevice graphicsDevice, ElemCommandQueueType type, const ElemCommandQueueOptions* options);
void VulkanFreeCommandQueue(ElemCommandQueue commandQueue);
ElemCommandList VulkanCreateCommandList(ElemCommandQueue commandQueue, const ElemCommandListOptions* options);
void VulkanCommitCommandList(ElemCommandList commandList);

ElemFence VulkanExecuteCommandList(ElemCommandQueue commandQueue, ElemCommandList commandList, const ElemExecuteCommandListOptions* options);
ElemFence VulkanExecuteCommandLists(ElemCommandQueue commandQueue, ElemCommandListSpan commandLists, const ElemExecuteCommandListOptions* options);
void VulkanWaitForFenceOnCpu(ElemFence fence);
