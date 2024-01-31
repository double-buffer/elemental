#pragma once

#include "VulkanGraphicsDevice.h"

struct VulkanCommandPoolItem;
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
        CurrentPipelineState = {};
        CurrentShader = nullptr;
    }

    VulkanGraphicsDevice* GraphicsDevice;
    VkCommandBuffer DeviceObject;
    VulkanCommandQueue* CommandQueue;
    bool IsFromCommandPool;
    bool IsRenderPassActive;
    VulkanCommandPoolItem* CommandPoolItem;
    RenderPassDescriptor CurrentRenderPassDescriptor;
    VulkanPipelineStateCacheItem CurrentPipelineState;
    VulkanShader* CurrentShader;
};
