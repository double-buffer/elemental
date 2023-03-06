#pragma once

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
    RenderPassDescriptor CurrentRenderPassDescriptor;
};