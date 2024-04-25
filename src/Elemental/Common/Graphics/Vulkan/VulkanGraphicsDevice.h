#pragma once

#include "Elemental.h"
#include "SystemMemory.h"

#ifdef WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#include "volk.h"

#define VULKAN_MAX_DEVICES 10u

struct VulkanGraphicsDeviceData
{
    VkDevice Device;
    VkPipelineLayout PipelineLayout;
    uint64_t CommandAllocationGeneration;
};

struct VulkanGraphicsDeviceDataFull
{
    VkPhysicalDevice PhysicalDevice;
    VkPhysicalDeviceProperties DeviceProperties;
    VkPhysicalDeviceMemoryProperties DeviceMemoryProperties;
    uint32_t RenderCommandQueueIndex;
    uint32_t CurrentRenderCommandQueueIndex;
    uint32_t ComputeCommandQueueIndex;
    uint32_t CurrentComputeCommandQueueIndex;
    uint32_t CopyCommandQueueIndex;
    uint32_t CurrentCopyCommandQueueIndex;
};

extern MemoryArena VulkanGraphicsMemoryArena;
extern VkInstance VulkanInstance;
extern bool VulkanDebugLayerEnabled;

VulkanGraphicsDeviceData* GetVulkanGraphicsDeviceData(ElemGraphicsDevice graphicsDevice);
VulkanGraphicsDeviceDataFull* GetVulkanGraphicsDeviceDataFull(ElemGraphicsDevice graphicsDevice);

void VulkanEnableGraphicsDebugLayer();
ElemGraphicsDeviceInfoSpan VulkanGetAvailableGraphicsDevices();
ElemGraphicsDevice VulkanCreateGraphicsDevice(const ElemGraphicsDeviceOptions* options);
void VulkanFreeGraphicsDevice(ElemGraphicsDevice graphicsDevice);
ElemGraphicsDeviceInfo VulkanGetGraphicsDeviceInfo(ElemGraphicsDevice graphicsDevice);
