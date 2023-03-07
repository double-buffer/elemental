#pragma once

struct VulkanCommandPoolItem;

struct VulkanCommandList : VulkanBaseGraphicsObject
{
    VulkanCommandList(BaseGraphicsService* graphicsService, VulkanGraphicsDevice* graphicsDevice) : VulkanBaseGraphicsObject(graphicsService, graphicsDevice)
    {
        IsFromCommandPool = false;
        CommandPoolItem = nullptr;
    }

    VkCommandBuffer DeviceObject;
    VulkanCommandQueue* CommandQueue;
    bool IsFromCommandPool;
    VulkanCommandPoolItem* CommandPoolItem;
    RenderPassDescriptor CurrentRenderPassDescriptor;
};