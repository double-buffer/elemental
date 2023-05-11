#pragma once

struct VulkanCommandPoolItem;
struct VulkanPipelineStateCacheItem;
struct VulkanShader;

struct VulkanCommandList : GraphicsObject
{
    VulkanCommandList(VulkanGraphicsDevice* graphicsDevice)
    {
        GraphicsDevice = graphicsDevice;
        GraphicsApi = GraphicsApi_Vulkan;
        IsFromCommandPool = false;
        IsRenderPassActive = false;
        CommandPoolItem = nullptr;
        CurrentPipelineState = nullptr;
        CurrentShader = nullptr;
    }

    VulkanGraphicsDevice* GraphicsDevice;
    VkCommandBuffer DeviceObject;
    VulkanCommandQueue* CommandQueue;
    bool IsFromCommandPool;
    bool IsRenderPassActive;
    VulkanCommandPoolItem* CommandPoolItem;
    RenderPassDescriptor CurrentRenderPassDescriptor;
    VulkanPipelineStateCacheItem* CurrentPipelineState;
    VulkanShader* CurrentShader;
};