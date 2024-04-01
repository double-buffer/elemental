#pragma once

#include "Elemental.h"

struct VulkanCommandQueueData
{
};

struct VulkanCommandQueueDataFull
{
};

struct VulkanCommandListData
{
};

struct VulkanCommandListDataFull
{
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
