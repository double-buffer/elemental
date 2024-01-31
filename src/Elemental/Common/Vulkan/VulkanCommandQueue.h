#pragma once

struct VulkanCommandQueue : GraphicsObject
{
    VulkanCommandQueue(VulkanGraphicsDevice* graphicsDevice)
    {
        GraphicsDevice = graphicsDevice;
        GraphicsApi = GraphicsApi_Vulkan;
    }

    VulkanGraphicsDevice* GraphicsDevice;
    VkQueue DeviceObject;
    CommandQueueType CommandQueueType;
    uint32_t FamilyIndex;

    VkSemaphore TimelineSemaphore;
    uint64_t FenceValue = 0;
    uint64_t LastCompletedFenceValue = 0;
};