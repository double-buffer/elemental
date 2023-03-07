#pragma once

struct VulkanSwapChain : VulkanBaseGraphicsObject
{
    VulkanSwapChain(BaseGraphicsService* graphicsService, VulkanGraphicsDevice* graphicsDevice) : VulkanBaseGraphicsObject(graphicsService, graphicsDevice)
    {
    }

    VkSwapchainKHR DeviceObject;
    VkSurfaceKHR WindowSurface;
    VulkanCommandQueue* CommandQueue;
    VkSwapchainCreateInfoKHR CreateInfo;
    SwapChainFormat Format;
    VulkanTexture* BackBufferTextures[3];
    uint32_t CurrentImageIndex = 0;
    uint64_t CurrentPresentId = 0;
    uint32_t MaximumFrameLatency = 0;
    VkFence BackBufferAcquireFence;
};