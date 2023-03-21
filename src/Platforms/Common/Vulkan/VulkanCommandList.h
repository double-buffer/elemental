#pragma once

struct VulkanCommandPoolItem;
struct VulkanPipelineStateCacheItem;
struct VulkanShader;

struct VulkanCommandList : VulkanBaseGraphicsObject
{
    VulkanCommandList(BaseGraphicsService* graphicsService, VulkanGraphicsDevice* graphicsDevice) : VulkanBaseGraphicsObject(graphicsService, graphicsDevice)
    {
        IsFromCommandPool = false;
        IsRenderPassActive = false;
        CommandPoolItem = nullptr;
        CurrentPipelineState = nullptr;
        CurrentShader = nullptr;
    }

    VkCommandBuffer DeviceObject;
    VulkanCommandQueue* CommandQueue;
    bool IsFromCommandPool;
    bool IsRenderPassActive;
    VulkanCommandPoolItem* CommandPoolItem;
    RenderPassDescriptor CurrentRenderPassDescriptor;
    VulkanPipelineStateCacheItem* CurrentPipelineState;
    VulkanShader* CurrentShader;
};