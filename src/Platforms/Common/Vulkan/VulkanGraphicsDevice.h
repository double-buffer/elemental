#pragma once
#ifdef _WINDOWS
#include "WindowsCommon.h"
#endif
#include "Volk/volk.h"
#include "../BaseGraphicsObject.h"

struct VulkanGraphicsDevice : BaseGraphicsObject
{
    VulkanGraphicsDevice(BaseGraphicsService* graphicsService) : BaseGraphicsObject(graphicsService)
    {
    }

    VkDevice Device;
    VkPhysicalDevice PhysicalDevice;
    VkPhysicalDeviceProperties DeviceProperties;
    VkPhysicalDeviceMemoryProperties DeviceMemoryProperties;

    uint32_t RenderCommandQueueFamilyIndex;
    uint32_t ComputeCommandQueueFamilyIndex;
    uint32_t CopyCommandQueueFamilyIndex;
};