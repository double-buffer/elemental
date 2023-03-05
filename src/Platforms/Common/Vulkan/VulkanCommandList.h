#pragma once
#ifdef _WINDOWS
#include "WindowsCommon.h"
#endif
#include "Volk/volk.h"
#include "VulkanBaseGraphicsObject.h"

struct VulkanCommandList : VulkanBaseGraphicsObject
{
    VulkanCommandList(BaseGraphicsService* graphicsService, VulkanGraphicsDevice* graphicsDevice) : VulkanBaseGraphicsObject(graphicsService, graphicsDevice)
    {
        IsFromCommandPool = false;
        IsUsed = true;
    }

    VkCommandBuffer DeviceObject;
    VulkanCommandQueue* CommandQueue;
    bool IsUsed;
    bool IsFromCommandPool;
};