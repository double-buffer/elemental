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

struct VulkanDescriptorSet
{
    VkDescriptorPool DescriptorPool;
    VkDescriptorSet DescriptorSet;
};

struct VulkanFreeListItem
{
    uint32_t Next;
};

struct VulkanDescriptorHeapStorage
{
    const VulkanDescriptorSet* DescriptorSet;
    Span<VulkanFreeListItem> Items;
    uint32_t CurrentIndex;
    uint32_t FreeListIndex;
};

struct VulkanDescriptorHeap
{
    VulkanDescriptorHeapStorage* Storage;
};

struct VulkanQueryHeapStorage
{
    VkQueryPool QueryHeap;
    VulkanGraphicsBufferCpu QueryHeapReadbackBuffer;
    VkQueryType Type;
    void* ReadbackCpuPointer;
    Span<VulkanFreeListItem> Items;
    uint32_t CurrentIndex;
    uint32_t InitializationIndex;
    uint32_t FreeListIndex;
};

struct VulkanQueryHeap
{
    VulkanQueryHeapStorage* Storage;
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
    Span<UploadBufferDevicePool<VulkanGraphicsBufferCpu>*> UploadBufferPools;
    uint32_t CurrentUploadBufferPoolIndex;
    VulkanQueryHeap QueryHeap;
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

uint32_t CreateVulkanQueryHeapIndex(VulkanQueryHeap queryHeap);
void FreeVulkanQueryHeapIndex(VulkanQueryHeap queryHeap, uint32_t index);

ElemGraphicsDeviceInfoSpan VulkanGetAvailableGraphicsDevices();
ElemGraphicsDevice VulkanCreateGraphicsDevice(const ElemGraphicsDeviceOptions* options);
void VulkanFreeGraphicsDevice(ElemGraphicsDevice graphicsDevice);
ElemGraphicsDeviceInfo VulkanGetGraphicsDeviceInfo(ElemGraphicsDevice graphicsDevice);
