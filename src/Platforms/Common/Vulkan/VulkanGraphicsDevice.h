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

    uint32_t InternalId;
    uint32_t RenderCommandQueueFamilyIndex;
    uint32_t ComputeCommandQueueFamilyIndex;
    uint32_t CopyCommandQueueFamilyIndex;
    
    CircularList<VulkanCommandPoolItem> DirectCommandPool;
    CircularList<VulkanCommandPoolItem> ComputeCommandPool;
    CircularList<VulkanCommandPoolItem> CopyCommandPool;
    uint64_t CommandPoolGeneration = 0;
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