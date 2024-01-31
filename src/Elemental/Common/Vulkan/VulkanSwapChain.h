#pragma once

struct VulkanSwapChain : GraphicsObject
{
    VulkanSwapChain(VulkanGraphicsDevice* graphicsDevice)
    {
        GraphicsDevice = graphicsDevice;
        GraphicsApi = GraphicsApi_Vulkan;

        BackBufferTextures[0] = nullptr;
        BackBufferTextures[1] = nullptr;
        BackBufferTextures[2] = nullptr;
    }

    VulkanGraphicsDevice* GraphicsDevice;
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