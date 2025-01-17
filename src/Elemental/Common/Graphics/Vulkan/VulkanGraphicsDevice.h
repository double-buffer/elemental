#pragma once

#include "Elemental.h"
#include "SystemMemory.h"
#include "VulkanResource.h"
#include "Graphics/UploadBufferPool.h"

#ifdef WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#elif __linux__
#define VK_USE_PLATFORM_WAYLAND_KHR
#endif
#include "volk.h"

#define VULKAN_MAX_DEVICES 10u

// TODO: Investigate why we cannot bump this to 1 million
//#define VULKAN_MAX_RESOURCES 400000
#define VULKAN_MAX_RESOURCES 1000000
#define VULKAN_MAX_SAMPLER 2048

struct VulkanDescriptorHeapStorage;

struct VulkanDescriptorSet;

struct VulkanDescriptorHeap
{
    VulkanDescriptorHeapStorage* Storage;
};

struct VulkanGraphicsDeviceData
{
    VkDevice Device;
    MemoryArena MemoryArena;
    VkPipelineLayout PipelineLayout;
    uint64_t CommandAllocationGeneration;
    uint64_t UploadBufferGeneration;
    VulkanDescriptorHeap ResourceDescriptorHeap;
    VulkanDescriptorHeap SamplerDescriptorHeap;
    Span<UploadBufferDevicePool<VulkanUploadBuffer>*> UploadBufferPools;
    uint32_t CurrentUploadBufferPoolIndex;
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
    uint32_t GpuMemoryTypeIndex;
    uint32_t GpuUploadMemoryTypeIndex;
    uint32_t ReadBackMemoryTypeIndex;
    uint32_t UploadMemoryTypeIndex;
    VkDescriptorSetLayout ResourceDescriptorSetLayout;
    VkDescriptorSetLayout SamplerDescriptorSetLayout;
};

extern MemoryArena VulkanGraphicsMemoryArena;
extern VkInstance VulkanInstance;
extern bool VulkanDebugLayerEnabled;
extern bool VulkanDebugBarrierInfoEnabled;

VulkanGraphicsDeviceData* GetVulkanGraphicsDeviceData(ElemGraphicsDevice graphicsDevice);
VulkanGraphicsDeviceDataFull* GetVulkanGraphicsDeviceDataFull(ElemGraphicsDevice graphicsDevice);

VkCompareOp ConvertToVulkanCompareFunction(ElemGraphicsCompareFunction compareFunction);

void VulkanSetGraphicsOptions(const ElemGraphicsOptions* options);

uint32_t CreateVulkanDescriptorHandle(VulkanDescriptorHeap descriptorHeap);
void FreeVulkanDescriptorHandle(VulkanDescriptorHeap descriptorHeap, uint32_t handle);

ElemGraphicsDeviceInfoSpan VulkanGetAvailableGraphicsDevices();
ElemGraphicsDevice VulkanCreateGraphicsDevice(const ElemGraphicsDeviceOptions* options);
void VulkanFreeGraphicsDevice(ElemGraphicsDevice graphicsDevice);
ElemGraphicsDeviceInfo VulkanGetGraphicsDeviceInfo(ElemGraphicsDevice graphicsDevice);
