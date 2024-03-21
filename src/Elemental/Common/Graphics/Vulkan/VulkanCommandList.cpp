#include "VulkanCommandList.h"
#include "VulkanGraphicsDevice.h"
#include "SystemDataPool.h"

#define VULKAN_MAX_COMMANDQUEUES 10u
#define VULKAN_MAX_COMMANDLISTS 64u

SystemDataPool<VulkanCommandQueueData, VulkanCommandQueueDataFull> vulkanCommandQueuePool;
SystemDataPool<VulkanCommandListData, VulkanCommandListDataFull> vulkanCommandListPool;

void InitVulkanCommandListMemory()
{
    if (!vulkanCommandQueuePool.Storage)
    {
        vulkanCommandQueuePool = SystemCreateDataPool<VulkanCommandQueueData, VulkanCommandQueueDataFull>(VulkanGraphicsMemoryArena, VULKAN_MAX_COMMANDQUEUES);
        vulkanCommandListPool = SystemCreateDataPool<VulkanCommandListData, VulkanCommandListDataFull>(VulkanGraphicsMemoryArena, VULKAN_MAX_COMMANDLISTS);
    }
}

VulkanCommandQueueData* GetVulkanCommandQueueData(ElemCommandQueue commandQueue)
{
    return SystemGetDataPoolItem(vulkanCommandQueuePool, commandQueue);
}

VulkanCommandQueueDataFull* GetVulkanCommandQueueDataFull(ElemCommandQueue commandQueue)
{
    return SystemGetDataPoolItemFull(vulkanCommandQueuePool, commandQueue);
}

VulkanCommandListData* GetVulkanCommandListData(ElemCommandList commandList)
{
    return SystemGetDataPoolItem(vulkanCommandListPool, commandList);
}

VulkanCommandListDataFull* GetVulkanCommandListDataFull(ElemCommandList commandList)
{
    return SystemGetDataPoolItemFull(vulkanCommandListPool, commandList);
}

ElemCommandQueue VulkanCreateCommandQueue(ElemGraphicsDevice graphicsDevice, ElemCommandQueueType type, const ElemCommandQueueOptions* options)
{
    return ELEM_HANDLE_NULL;
}

void VulkanFreeCommandQueue(ElemCommandQueue commandQueue)
{
}

ElemCommandList VulkanCreateCommandList(ElemCommandQueue commandQueue, const ElemCommandListOptions* options)
{
    return ELEM_HANDLE_NULL;
}

void VulkanCommitCommandList(ElemCommandList commandList)
{
}

ElemFence VulkanExecuteCommandLists(ElemCommandQueue commandQueue, ElemCommandListSpan commandLists, const ElemExecuteCommandListOptions* options)
{
    return {};
}

void VulkanWaitForFenceOnCpu(ElemFence fence)
{
}
