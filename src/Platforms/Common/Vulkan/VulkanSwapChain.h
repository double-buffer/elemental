#pragma once
#ifdef _WINDOWS
#include "WindowsCommon.h"
#endif
#include "VulkanBaseGraphicsObject.h"

struct VulkanSwapChain : VulkanBaseGraphicsObject
{
    VulkanSwapChain(BaseGraphicsService* graphicsService, VulkanGraphicsDevice* graphicsDevice) : VulkanBaseGraphicsObject(graphicsService, graphicsDevice)
    {
    }

    VkSwapchainKHR DeviceObject;
    VkSurfaceKHR WindowSurface;
    VulkanCommandQueue* CommandQueue;
    SwapChainFormat Format;
    VulkanTexture* BackBufferTextures[3];
    uint32_t CurrentImageIndex;

    /*VkFormat Format;
    VkFence BackBufferAcquireFence;*/
};