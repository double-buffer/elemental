#pragma once

struct VulkanCommandPoolItem
{
    VulkanCommandPoolItem()
    {
        CommandPool = nullptr;

        for (uint32_t i = 0; i < MAX_VULKAN_COMMAND_BUFFERS; i++)
        {
            CommandLists[i] = nullptr;
        }

        CurrentCommandListIndex = 0;
        IsInUse = true;
    }

    VkCommandPool CommandPool;
    Fence Fence;
    VulkanCommandList* CommandLists[MAX_VULKAN_COMMAND_BUFFERS];
    uint32_t CurrentCommandListIndex;
    bool IsInUse;
};

struct VulkanPipelineStateCacheItem
{
    VkPipeline PipelineState;
};

struct VulkanGraphicsDevice : BaseGraphicsObject
{
    VulkanGraphicsDevice(BaseGraphicsService* graphicsService) : BaseGraphicsObject(graphicsService),
        DirectCommandPool(MAX_VULKAN_COMMAND_POOLS), 
        ComputeCommandPool(MAX_VULKAN_COMMAND_POOLS), 
        CopyCommandPool(MAX_VULKAN_COMMAND_POOLS)
    {
    }

    VkDevice Device;
    VkPhysicalDevice PhysicalDevice;
    VkPhysicalDeviceProperties DeviceProperties;
    VkPhysicalDeviceMemoryProperties DeviceMemoryProperties;

    uint32_t InternalId = UINT32_MAX;
    uint32_t RenderCommandQueueFamilyIndex = UINT32_MAX;
    uint32_t ComputeCommandQueueFamilyIndex = UINT32_MAX;
    uint32_t CopyCommandQueueFamilyIndex = UINT32_MAX;
    
    CircularList<VulkanCommandPoolItem> DirectCommandPool;
    CircularList<VulkanCommandPoolItem> ComputeCommandPool;
    CircularList<VulkanCommandPoolItem> CopyCommandPool;
    uint64_t CommandPoolGeneration = 0;
    
    Dictionary<uint64_t, VulkanPipelineStateCacheItem> PipelineStates;
};

struct VulkanDeviceCommandPools
{
    uint64_t Generation = 0;
    VulkanCommandPoolItem* DirectCommandPool = nullptr;
    VulkanCommandPoolItem* ComputeCommandPool = nullptr;
    VulkanCommandPoolItem* CopyCommandPool = nullptr;

    bool IsEmpty()
    {
        return DirectCommandPool == nullptr;
    }

    void Reset(uint64_t currentGeneration)
    {
        DirectCommandPool = nullptr;
        ComputeCommandPool = nullptr;
        CopyCommandPool = nullptr;
        Generation = currentGeneration;
    }
};