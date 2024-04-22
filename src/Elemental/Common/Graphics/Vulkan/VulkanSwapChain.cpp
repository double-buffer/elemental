#include "VulkanSwapChain.h"
#include "VulkanGraphicsDevice.h"
#include "VulkanCommandList.h"
#include "VulkanTexture.h"
#include "Win32Application.h"
#include "Win32Window.h"
#include "SystemDataPool.h"
#include "SystemFunctions.h"
#include "SystemMemory.h"

#define VULKAN_MAX_SWAPCHAINS 10u

SystemDataPool<VulkanSwapChainData, VulkanSwapChainDataFull> vulkanSwapChainPool;

void InitVulkanSwapChainMemory()
{
    if (!vulkanSwapChainPool.Storage)
    {
        vulkanSwapChainPool = SystemCreateDataPool<VulkanSwapChainData, VulkanSwapChainDataFull>(VulkanGraphicsMemoryArena, VULKAN_MAX_SWAPCHAINS);
    }
}

VulkanSwapChainData* GetVulkanSwapChainData(ElemSwapChain swapChain)
{
    return SystemGetDataPoolItem(vulkanSwapChainPool, swapChain);
}

VulkanSwapChainDataFull* GetVulkanSwapChainDataFull(ElemSwapChain swapChain)
{
    return SystemGetDataPoolItemFull(vulkanSwapChainPool, swapChain);
}

void CreateVulkanSwapChainBackBuffers(ElemSwapChain swapChain, uint32_t width, uint32_t height)
{
    SystemAssert(swapChain != ELEM_HANDLE_NULL);

    auto stackMemoryArena = SystemGetStackMemoryArena();

    auto swapChainData = GetVulkanSwapChainData(swapChain);
    SystemAssert(swapChainData);

    auto swapChainDataFull = GetVulkanSwapChainDataFull(swapChain);
    SystemAssert(swapChainDataFull);

    auto graphicsDeviceData = GetVulkanGraphicsDeviceData(swapChainData->GraphicsDevice);
    SystemAssert(graphicsDeviceData);

    uint32_t swapchainImageCount = 0;
    AssertIfFailed(vkGetSwapchainImagesKHR(graphicsDeviceData->Device, swapChainData->DeviceObject, &swapchainImageCount, nullptr));

    auto swapchainImages = SystemPushArray<VkImage>(stackMemoryArena, swapchainImageCount);
    AssertIfFailed(vkGetSwapchainImagesKHR(graphicsDeviceData->Device, swapChainData->DeviceObject, &swapchainImageCount, swapchainImages.Pointer));

    for (uint32_t i = 0; i < swapchainImageCount; i++)
    {
        // TODO: FreeTexture in resize
        swapChainData->BackBufferTextures[i] = CreateVulkanTextureFromResource(swapChainData->GraphicsDevice, swapchainImages[i], swapChainDataFull->VulkanFormat, true);
    }
}

void CheckVulkanAvailableSwapChain(ElemHandle handle)
{
    SystemAssert(handle != ELEM_HANDLE_NULL);

    auto stackMemoryArena = SystemGetStackMemoryArena();

    auto swapChainData = GetVulkanSwapChainData(handle);
    SystemAssert(swapChainData);

    auto graphicsDeviceData = GetVulkanGraphicsDeviceData(swapChainData->GraphicsDevice);
    SystemAssert(graphicsDeviceData);

    if (swapChainData->PresentId > swapChainData->FrameLatency)
    {
        auto presentId = swapChainData->PresentId - swapChainData->FrameLatency;

        if (vkWaitForPresentKHR(graphicsDeviceData->Device, swapChainData->DeviceObject, presentId, 0) != VK_SUCCESS)
        {
            return;
        }
    }

    SystemLogDebugMessage(ElemLogMessageCategory_Graphics, "Updating frame");
/*
    ElemWindowSize windowSize = ElemGetWindowRenderSize(swapChainData->Window);

    if (windowSize.Width > 0 && windowSize.Height > 0 && (windowSize.Width != swapChainData->Width || windowSize.Height != swapChainData->Height))
    {
        ResizeDirectX12SwapChain(handle, windowSize.Width, windowSize.Height);
    }*/

    AssertIfFailed(vkAcquireNextImageKHR(graphicsDeviceData->Device, swapChainData->DeviceObject, UINT64_MAX, VK_NULL_HANDLE, swapChainData->BackBufferAcquireFence, &swapChainData->CurrentImageIndex));
    vkWaitForFences(graphicsDeviceData->Device, 1, &swapChainData->BackBufferAcquireFence, true, UINT64_MAX);
    vkResetFences(graphicsDeviceData->Device, 1, &swapChainData->BackBufferAcquireFence);

    swapChainData->PresentCalled = false;
    auto backBufferTexture = swapChainData->BackBufferTextures[swapChainData->CurrentImageIndex];

    ElemSwapChainUpdateParameters updateParameters
    {
        .SwapChainInfo = VulkanGetSwapChainInfo(handle),
        .BackBufferTexture = backBufferTexture,
        .DeltaTimeInSeconds = 1.0f / 120.0f,//deltaTime,
        .NextPresentTimeStampInSeconds = 1.0f//nextPresentTimeStampInSeconds
    };
    
    swapChainData->UpdateHandler(&updateParameters, swapChainData->UpdatePayload);
/*
    if (!swapChainData->PresentCalled)
    {
        SystemLogWarningMessage(ElemLogMessageCategory_Graphics, "Present was not called during update.");
        DirectX12PresentSwapChain(handle);
    }*/

}

ElemSwapChain VulkanCreateSwapChain(ElemCommandQueue commandQueue, ElemWindow window, ElemSwapChainUpdateHandlerPtr updateHandler, const ElemSwapChainOptions* options)
{
    auto stackMemoryArena = SystemGetStackMemoryArena();

    InitVulkanSwapChainMemory();
    SystemAssert(commandQueue != ELEM_HANDLE_NULL);
    
    auto commandQueueData = GetVulkanCommandQueueData(commandQueue);
    SystemAssert(commandQueueData);

    auto commandQueueDataFull = GetVulkanCommandQueueDataFull(commandQueue);
    SystemAssert(commandQueueDataFull);

    auto graphicsDeviceData = GetVulkanGraphicsDeviceData(commandQueueData->GraphicsDevice);
    SystemAssert(graphicsDeviceData);

    auto graphicsDeviceDataFull = GetVulkanGraphicsDeviceDataFull(commandQueueData->GraphicsDevice);
    SystemAssert(graphicsDeviceDataFull);

    auto windowData = GetWin32WindowData(window);
    SystemAssert(windowData);

    #ifdef _WINDOWS
    VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = { VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
    surfaceCreateInfo.hinstance = GetModuleHandle(nullptr);
    surfaceCreateInfo.hwnd = windowData->WindowHandle;

    VkSurfaceKHR windowSurface;
    AssertIfFailed(vkCreateWin32SurfaceKHR(VulkanInstance, &surfaceCreateInfo, nullptr, &windowSurface));
    #endif

    VkBool32 isPresentSupported;
    AssertIfFailed(vkGetPhysicalDeviceSurfaceSupportKHR(graphicsDeviceDataFull->PhysicalDevice, commandQueueData->QueueFamilyIndex, windowSurface, &isPresentSupported));
    SystemAssert(isPresentSupported);

    VkSurfaceCapabilitiesKHR surfaceCapabilities = {};
    AssertIfFailed(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(graphicsDeviceDataFull->PhysicalDevice, windowSurface, &surfaceCapabilities));

    uint32_t surfaceFormatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(graphicsDeviceDataFull->PhysicalDevice, windowSurface, &surfaceFormatCount, nullptr);

    auto surfaceFormats = SystemPushArray<VkSurfaceFormatKHR>(stackMemoryArena, surfaceFormatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(graphicsDeviceDataFull->PhysicalDevice, windowSurface, &surfaceFormatCount, surfaceFormats.Pointer);
    SystemAssert(surfaceFormatCount > 0);

    auto windowRenderSize = ElemGetWindowRenderSize(window);
    auto width = windowRenderSize.Width;
    auto height = windowRenderSize.Height;
    auto format = VK_FORMAT_B8G8R8A8_SRGB; // TODO: Enumerate compatible formats first
    auto colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    auto frameLatency = 1u;

    auto targetFPS = windowData->MonitorRefreshRate;
    void* updatePayload = nullptr;
 
    if (options)
    {
        if (options->Format == ElemSwapChainFormat_HighDynamicRange)
        {
            format = VK_FORMAT_A2B10G10R10_UNORM_PACK32;
            colorSpace = VK_COLOR_SPACE_HDR10_ST2084_EXT;
        }

        // TODO: Check boundaries
        if (options->FrameLatency != 0)
        {
            frameLatency = options->FrameLatency;
        }

        if (options->TargetFPS != 0)
        {
            targetFPS = options->TargetFPS;
        }

        if (options->UpdatePayload)
        {
            updatePayload = options->UpdatePayload;
        }
    }

    VkSwapchainCreateInfoKHR swapChainCreateInfo = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
    swapChainCreateInfo.surface = windowSurface;
    swapChainCreateInfo.minImageCount = VULKAN_MAX_SWAPCHAIN_BUFFERS;
    swapChainCreateInfo.imageFormat = format;
    swapChainCreateInfo.imageColorSpace = colorSpace;
    swapChainCreateInfo.imageArrayLayers = 1;
    swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapChainCreateInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    swapChainCreateInfo.preTransform = surfaceCapabilities.currentTransform;
    swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapChainCreateInfo.flags = 0;
    swapChainCreateInfo.imageExtent.width = width;
    swapChainCreateInfo.imageExtent.height = height;

    VkSwapchainKHR swapChain;
    AssertIfFailed(vkCreateSwapchainKHR(graphicsDeviceData->Device, &swapChainCreateInfo, nullptr, &swapChain));

    VkFence acquireFence;
    VkFenceCreateInfo fenceCreateInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    AssertIfFailed(vkCreateFence(graphicsDeviceData->Device, &fenceCreateInfo, 0, &acquireFence ));

    LARGE_INTEGER creationTimestamp;
    QueryPerformanceCounter(&creationTimestamp);

    auto handle = SystemAddDataPoolItem(vulkanSwapChainPool, {
        .DeviceObject = swapChain,
        .WindowSurface = windowSurface,
        .GraphicsDevice = commandQueueData->GraphicsDevice,
        .CommandQueue = commandQueue,
        .Window = window,
        .BackBufferAcquireFence = acquireFence,
        .UpdateHandler = updateHandler,
        .UpdatePayload = updatePayload,
        .CreationTimestamp = creationTimestamp,
        .PreviousTargetPresentationTimestamp = {},
        .Width = width,
        .Height = height,
        .Format = ElemTextureFormat_B8G8R8A8_SRGB, // TODO: change that
        .FrameLatency = frameLatency,
        .TargetFPS = targetFPS
    }); 

    SystemAddDataPoolItemFull(vulkanSwapChainPool, handle, {
        .VulkanFormat = format
    });

    CreateVulkanSwapChainBackBuffers(handle, width, height);
    AddWin32RunLoopHandler(CheckVulkanAvailableSwapChain, handle);

    return handle;
}

void VulkanFreeSwapChain(ElemSwapChain swapChain)
{
}

ElemSwapChainInfo VulkanGetSwapChainInfo(ElemSwapChain swapChain)
{
    return {};
}

void VulkanSetSwapChainTiming(ElemSwapChain swapChain, uint32_t frameLatency, uint32_t targetFPS)
{
}

void VulkanPresentSwapChain(ElemSwapChain swapChain)
{
    SystemAssert(swapChain != ELEM_HANDLE_NULL);

    auto swapChainData = GetVulkanSwapChainData(swapChain);
    SystemAssert(swapChainData);

    auto swapChainDataFull = GetVulkanSwapChainDataFull(swapChain);
    SystemAssert(swapChainDataFull);
    
    auto commandQueueData = GetVulkanCommandQueueData(swapChainData->CommandQueue);
    SystemAssert(commandQueueData);

    // HACK: We should set the release semaphore to the latest command buffer
    // Otherwise, the results maybe undefined?

    VkPresentIdKHR presentIdInfo = { VK_STRUCTURE_TYPE_PRESENT_ID_KHR };
    presentIdInfo.swapchainCount = 1;
    presentIdInfo.pPresentIds = &swapChainData->PresentId;

    VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
    presentInfo.waitSemaphoreCount = 0;
    presentInfo.pWaitSemaphores = nullptr;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapChainData->DeviceObject;
    presentInfo.pImageIndices = &swapChainData->CurrentImageIndex;
    presentInfo.pNext = &presentIdInfo;

    AssertIfFailed(vkQueuePresentKHR(commandQueueData->DeviceObject, &presentInfo));
    
    //VulkanResetCommandAllocation(swapChain->GraphicsDevice);
    swapChainData->PresentId++;
}

