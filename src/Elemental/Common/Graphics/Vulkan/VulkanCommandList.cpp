#include "VulkanCommandList.h"
#include "VulkanGraphicsDevice.h"
#include "SystemDataPool.h"

#define VULKAN_MAX_COMMANDQUEUES 10u

SystemDataPool<VulkanCommandQueueData, VulkanCommandQueueDataFull> vulkanCommandQueuePool;

void InitVulkanCommandListMemory()
{
    if (!vulkanCommandQueuePool.Storage)
    {
        vulkanCommandQueuePool = SystemCreateDataPool<VulkanCommandQueueData, VulkanCommandQueueDataFull>(VulkanGraphicsMemoryArena, VULKAN_MAX_COMMANDQUEUES);
    }
}

VulkanCommandQueueData* GetVulkanCommandQueueData(ElemCommandQueue graphicsDevice)
{
    return SystemGetDataPoolItem(vulkanCommandQueuePool, graphicsDevice);
}

VulkanCommandQueueDataFull* GetVulkanCommandQueueDataFull(ElemCommandQueue graphicsDevice)
{
    return SystemGetDataPoolItemFull(vulkanCommandQueuePool, graphicsDevice);
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

ElemFence VulkanExecuteCommandList(ElemCommandQueue commandQueue, ElemCommandList commandList, const ElemExecuteCommandListOptions* options)
{
    return ELEM_HANDLE_NULL;
}

ElemFence VulkanExecuteCommandLists(ElemCommandQueue commandQueue, ElemCommandListSpan commandLists, const ElemExecuteCommandListOptions* options)
{
    return ELEM_HANDLE_NULL;
}

void VulkanWaitForFenceOnCpu(ElemFence fence)
{
}
