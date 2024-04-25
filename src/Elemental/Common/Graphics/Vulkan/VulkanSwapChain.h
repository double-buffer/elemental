#pragma once

#include "Elemental.h"
#define VK_USE_PLATFORM_WIN32_KHR
#include "volk.h"

#define VULKAN_MAX_SWAPCHAIN_BUFFERS 3

struct VulkanSwapChainData
{
    VkSwapchainKHR DeviceObject;
    VkSurfaceKHR WindowSurface;
    ElemGraphicsDevice GraphicsDevice;
    ElemCommandQueue CommandQueue;
    ElemWindow Window;
    ElemTexture BackBufferTextures[VULKAN_MAX_SWAPCHAIN_BUFFERS];
    uint32_t CurrentImageIndex;
    VkFence BackBufferAcquireFence;
    ElemSwapChainUpdateHandlerPtr UpdateHandler;
    void* UpdatePayload;
    LARGE_INTEGER CreationTimestamp;
    LARGE_INTEGER PreviousTargetPresentationTimestamp;
    uint32_t Width;
    uint32_t Height;
    ElemTextureFormat Format;
    bool PresentCalled;
    uint64_t PresentId;
    uint32_t FrameLatency;
    uint32_t TargetFPS;
};

struct VulkanSwapChainDataFull
{
    VkFormat VulkanFormat;
    VkSwapchainCreateInfoKHR CreateInfo;
};

VulkanSwapChainData* GetVulkanSwapChainData(ElemSwapChain swapChain);
VulkanSwapChainDataFull* GetVulkanSwapChainDataFull(ElemSwapChain swapChain);

ElemSwapChain VulkanCreateSwapChain(ElemCommandQueue commandQueue, ElemWindow window, ElemSwapChainUpdateHandlerPtr updateHandler, const ElemSwapChainOptions* options);
void VulkanFreeSwapChain(ElemSwapChain swapChain);
ElemSwapChainInfo VulkanGetSwapChainInfo(ElemSwapChain swapChain);
void VulkanSetSwapChainTiming(ElemSwapChain swapChain, uint32_t frameLatency, uint32_t targetFPS);
void VulkanPresentSwapChain(ElemSwapChain swapChain);
