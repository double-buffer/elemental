#include "VulkanCommandList.h"
#include "VulkanGraphicsDevice.h"
#include "SystemDataPool.h"

#define VULKAN_MAX_COMMANDQUEUES 10u

SystemDataPool<VulkanGraphicsCommandQueueData, VulkanGraphicsCommandQueueDataFull> vulkanGraphicsCommandQueuePool;

void InitVulkanCommandListMemory()
{
    if (!vulkanGraphicsCommandQueuePool.Storage)
    {
        vulkanGraphicsCommandQueuePool = SystemCreateDataPool<VulkanGraphicsCommandQueueData, VulkanGraphicsCommandQueueDataFull>(VulkanGraphicsMemoryArena, VULKAN_MAX_COMMANDQUEUES);
    }
}

VulkanGraphicsCommandQueueData* GetVulkanGraphicsCommandQueueData(ElemGraphicsCommandQueue graphicsDevice)
{
    return SystemGetDataPoolItem(vulkanGraphicsCommandQueuePool, graphicsDevice);
}

VulkanGraphicsCommandQueueDataFull* GetVulkanGraphicsCommandQueueDataFull(ElemGraphicsCommandQueue graphicsDevice)
{
    return SystemGetDataPoolItemFull(vulkanGraphicsCommandQueuePool, graphicsDevice);
}

ElemGraphicsCommandQueue VulkanCreateGraphicsCommandQueue(ElemGraphicsDevice graphicsDevice, ElemGraphicsCommandQueueType type, const ElemGraphicsCommandQueueOptions* options)
{
    return ELEM_HANDLE_NULL;
}

void VulkanFreeGraphicsCommandQueue(ElemGraphicsCommandQueue commandQueue)
{
}
