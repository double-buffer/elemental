#pragma once

#include "Elemental.h"
#include "SystemMemory.h"

#ifdef WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#ifdef __APPLE__

#endif
#include "volk.h"

struct VulkanGraphicsDeviceData
{
    VkDevice Device;
};

struct VulkanGraphicsDeviceDataFull
{
    VkPhysicalDevice PhysicalDevice;
    VkPhysicalDeviceProperties DeviceProperties;
    VkPhysicalDeviceMemoryProperties DeviceMemoryProperties;
};

extern MemoryArena VulkanGraphicsMemoryArena;

VulkanGraphicsDeviceData* GetVulkanGraphicsDeviceData(ElemGraphicsDevice graphicsDevice);
VulkanGraphicsDeviceDataFull* GetVulkanGraphicsDeviceDataFull(ElemGraphicsDevice graphicsDevice);

void VulkanEnableGraphicsDebugLayer();
ElemGraphicsDeviceInfoList VulkanGetAvailableGraphicsDevices();
ElemGraphicsDevice VulkanCreateGraphicsDevice(const ElemGraphicsDeviceOptions* options);
void VulkanFreeGraphicsDevice(ElemGraphicsDevice graphicsDevice);
ElemGraphicsDeviceInfo VulkanGetGraphicsDeviceInfo(ElemGraphicsDevice graphicsDevice);
