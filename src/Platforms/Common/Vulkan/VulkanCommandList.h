#pragma once

struct VulkanCommandPoolItem;
struct VulkanPipelineStateCacheItem;

struct VulkanCommandList : VulkanBaseGraphicsObject
{
    VulkanCommandList(BaseGraphicsService* graphicsService, VulkanGraphicsDevice* graphicsDevice) : VulkanBaseGraphicsObject(graphicsService, graphicsDevice)
    {
        IsFromCommandPool = false;
        IsRenderPassActive = false;
        CommandPoolItem = nullptr;
        CurrentPipelineState = nullptr;
    }

    VkCommandBuffer DeviceObject;
    VulkanCommandQueue* CommandQueue;
    bool IsFromCommandPool;
    bool IsRenderPassActive;
    VulkanCommandPoolItem* CommandPoolItem;
    RenderPassDescriptor CurrentRenderPassDescriptor;
    VulkanPipelineStateCacheItem* CurrentPipelineState;
};