#pragma once
#ifdef _WINDOWS
#include "WindowsCommon.h"
#endif
#include "VulkanBaseGraphicsObject.h"

struct VulkanCommandQueue : VulkanBaseGraphicsObject
{
    VulkanCommandQueue(BaseGraphicsService* graphicsService, VulkanGraphicsDevice* graphicsDevice) : VulkanBaseGraphicsObject(graphicsService, graphicsDevice)
    {
    }

    VkQueue DeviceObject;
    CommandQueueType CommandQueueType;
    uint32_t FamilyIndex;

    VkSemaphore TimelineSemaphore;
    uint64_t FenceValue = 0;
};