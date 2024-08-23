#include "VulkanSwapChain.h"
#include "VulkanGraphicsDevice.h"
#include "VulkanCommandList.h"
#include "VulkanResource.h"
#include "Inputs/Inputs.h"
#include "SystemDataPool.h"
#include "SystemFunctions.h"
#include "SystemMemory.h"

#ifdef _WIN32
#include "../Elemental/Microsoft/Win32Application.h"
#include "../Elemental/Microsoft/Win32Window.h"
#else
#include "../Elemental/Linux/WaylandApplication.h"
#include "../Elemental/Linux/WaylandWindow.h"
#endif

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
        ElemGraphicsResourceInfo resourceInfo = 
        {
            .Type = ElemGraphicsResourceType_Texture2D,
            .Width = width,
            .Height = height,
            .MipLevels = 1,
            .Format = swapChainData->Format,
            .Usage = ElemGraphicsResourceUsage_RenderTarget
        };

        swapChainData->BackBufferTextures[i] = CreateVulkanTextureFromResource(swapChainData->GraphicsDevice, swapchainImages[i], &resourceInfo, true);
    }
}

VkSwapchainKHR CreateVulkanSwapChainObject(ElemGraphicsDevice graphicsDevice, VkSurfaceKHR windowSurface, VkSwapchainCreateInfoKHR* swapChainCreateInfo, VkSwapchainKHR oldSwapChain)
{
    auto graphicsDeviceData = GetVulkanGraphicsDeviceData(graphicsDevice);
    SystemAssert(graphicsDeviceData);

    auto graphicsDeviceDataFull = GetVulkanGraphicsDeviceDataFull(graphicsDevice);
    SystemAssert(graphicsDeviceDataFull);

    VkSurfaceCapabilitiesKHR surfaceCapabilities = {};
    AssertIfFailed(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(graphicsDeviceDataFull->PhysicalDevice, windowSurface, &surfaceCapabilities));

    VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

    if (surfaceCapabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR)
    {
        compositeAlpha = VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR;
    }
    else if (surfaceCapabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR)
    {
        compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
    }

    // TODO: We have still some errors sometimes (Suboptimal)
    swapChainCreateInfo->imageArrayLayers = 1;
    swapChainCreateInfo->imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapChainCreateInfo->imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapChainCreateInfo->presentMode = VK_PRESENT_MODE_FIFO_KHR;
    swapChainCreateInfo->preTransform = surfaceCapabilities.currentTransform;
    swapChainCreateInfo->compositeAlpha = compositeAlpha;
    swapChainCreateInfo->flags = 0;
    swapChainCreateInfo->oldSwapchain = oldSwapChain;

    VkSwapchainKHR swapChain;
    AssertIfFailed(vkCreateSwapchainKHR(graphicsDeviceData->Device, swapChainCreateInfo, nullptr, &swapChain));


    return swapChain;
}

void ResizeVulkanSwapChain(ElemSwapChain swapChain, uint32_t width, uint32_t height)
{
    if (width == 0 || height == 0)
    {
        return;
    }
    
    SystemLogDebugMessage(ElemLogMessageCategory_Graphics, "Resize Swapchain to %dx%d.", width, height);
    SystemAssert(swapChain != ELEM_HANDLE_NULL);

    auto swapChainData = GetVulkanSwapChainData(swapChain);
    SystemAssert(swapChainData);

    auto swapChainDataFull = GetVulkanSwapChainDataFull(swapChain);
    SystemAssert(swapChainDataFull);

    auto graphicsDeviceData = GetVulkanGraphicsDeviceData(swapChainData->GraphicsDevice);
    SystemAssert(graphicsDeviceData);

    auto graphicsDeviceDataFull = GetVulkanGraphicsDeviceDataFull(swapChainData->GraphicsDevice);
    SystemAssert(graphicsDeviceDataFull);

    swapChainData->Width = width;
    swapChainData->Height = height;
    swapChainData->AspectRatio = (float)width / height;

    auto fence = CreateVulkanCommandQueueFence(swapChainData->CommandQueue);
    VulkanWaitForFenceOnCpu(fence);

    swapChainData->PresentId = 0;
    auto oldSwapChain = swapChainData->DeviceObject;

    auto swapChainCreateInfo = &swapChainDataFull->CreateInfo;
    swapChainCreateInfo->imageExtent.width = width;
    swapChainCreateInfo->imageExtent.height = height;
    
    // BUG: The resizing works well but not fullscreen! (Windows / Linux?)
    swapChainData->DeviceObject = CreateVulkanSwapChainObject(swapChainData->CommandQueue, swapChainData->WindowSurface, swapChainCreateInfo, oldSwapChain);

    for (uint32_t i = 0; i < VULKAN_MAX_SWAPCHAIN_BUFFERS; i++)
    {
        auto texture = swapChainData->BackBufferTextures[i];
        VulkanFreeGraphicsResource(texture, nullptr);
    }

    vkDestroySwapchainKHR(graphicsDeviceData->Device, oldSwapChain, nullptr);
    CreateVulkanSwapChainBackBuffers(swapChain, swapChainCreateInfo->imageExtent.width, swapChainCreateInfo->imageExtent.height);
}

void CheckVulkanAvailableSwapChain(ElemHandle handle)
{
    SystemAssert(handle != ELEM_HANDLE_NULL);

    auto swapChainData = GetVulkanSwapChainData(handle);
    SystemAssert(swapChainData);

    #ifdef _WIN32
    auto windowData = GetWin32WindowData(swapChainData->Window);
    #elif __linux__
    auto windowData = GetWaylandWindowData(swapChainData->Window);
    #endif

    SystemAssert(windowData);

    auto graphicsDeviceData = GetVulkanGraphicsDeviceData(swapChainData->GraphicsDevice);
    SystemAssert(graphicsDeviceData);

    if (swapChainData->PresentId > swapChainData->FrameLatency)
    {
        // TODO: This is only needed on win32? Normally on wayland even if use an update handle
        // We should be able to call this function
        // BUG: On win32, it seems laggy maybe something is wrong here. (We get 144 fps but it seems laggy)
        // It only happens on the first launch and for some reason if we go fullscreen it is fluid.
        // Looks like a windowed issue where we miss the vsync
        // The issue seems to be resolved for now... to monitor!
        #ifdef _WIN32
        auto presentId = swapChainData->PresentId - swapChainData->FrameLatency;

        if (vkWaitForPresentKHR(graphicsDeviceData->Device, swapChainData->DeviceObject, presentId, 0) != VK_SUCCESS)
        {
            return;
        }
        #endif
    }
    
    // TODO: Compute timing information
    // TODO: If delta time is above a thresold, take the delta time based on target FPS
    auto deltaTime = 1.0f / windowData->MonitorRefreshRate;

    ElemWindowSize windowSize = ElemGetWindowRenderSize(swapChainData->Window);

    auto sizeChanged = false;

    if (windowSize.Width > 0 && windowSize.Height > 0 && (windowSize.Width != swapChainData->Width || windowSize.Height != swapChainData->Height))
    {
        ResizeVulkanSwapChain(handle, windowSize.Width, windowSize.Height);
        sizeChanged = true;
    }

    AssertIfFailed(vkAcquireNextImageKHR(graphicsDeviceData->Device, swapChainData->DeviceObject, UINT64_MAX, VK_NULL_HANDLE, swapChainData->BackBufferAcquireFence, &swapChainData->CurrentImageIndex));
    vkWaitForFences(graphicsDeviceData->Device, 1, &swapChainData->BackBufferAcquireFence, true, UINT64_MAX);
    vkResetFences(graphicsDeviceData->Device, 1, &swapChainData->BackBufferAcquireFence);

    swapChainData->PresentCalled = false;
    auto backBuffer = swapChainData->BackBufferTextures[swapChainData->CurrentImageIndex];

    ElemSwapChainUpdateParameters updateParameters
    {
        .SwapChainInfo = VulkanGetSwapChainInfo(handle),
        .BackBufferRenderTarget = backBuffer,
        .DeltaTimeInSeconds = deltaTime,
        .NextPresentTimestampInSeconds = 1.0f,//nextPresentTimestampInSeconds
        .SizeChanged = sizeChanged
    };
    
    swapChainData->UpdateHandler(&updateParameters, swapChainData->UpdatePayload);
    ResetInputsFrame();
/*
    if (!swapChainData->PresentCalled)
    {
        SystemLogWarningMessage(ElemLogMessageCategory_Graphics, "Present was not called during update.");
        DirectX12PresentSwapChain(handle);
    }*/

}

#ifdef __linux__
struct WaylandCallbackParameters
{
    ElemSwapChain SwapChain;
    ElemWindow Window;
    wl_surface* WaylandSurface;
};

void RegisterWaylandFrameCallback(WaylandCallbackParameters* parameters);

static void WaylandFrameCallback(void* data, wl_callback* callback, uint32_t time) 
{
    auto parameters = (WaylandCallbackParameters*)data;


    // TODO: investigate frame time
    // See: https://gitlab.gnome.org/GNOME/gtk/-/blob/main/gdk/wayland/gdksurface-wayland.c
    //SystemLogDebugMessage(ElemLogMessageCategory_Graphics, "Wayland callback: %u", time);

    CheckVulkanAvailableSwapChain(parameters->SwapChain);
    
    auto windowData = GetWaylandWindowData(parameters->Window);

    if (windowData->IsClosed)
    {
        ElemExitApplication(0);
        return;
    }
    else
    {
        wl_callback_destroy(callback);
        RegisterWaylandFrameCallback(parameters);
    }
}

static wl_callback_listener frame_listener = { WaylandFrameCallback };

void RegisterWaylandFrameCallback(WaylandCallbackParameters* parameters)
{
    auto callback = wl_surface_frame(parameters->WaylandSurface);
    SystemAssert(callback);

    wl_callback_add_listener(callback, &frame_listener, parameters);
    wl_surface_commit(parameters->WaylandSurface);
}
#endif

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

    #ifdef _WIN32
    auto windowData = GetWin32WindowData(window);
    SystemAssert(windowData);

    VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = { VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
    surfaceCreateInfo.hinstance = GetModuleHandle(nullptr);
    surfaceCreateInfo.hwnd = windowData->WindowHandle;

    VkSurfaceKHR windowSurface;
    AssertIfFailed(vkCreateWin32SurfaceKHR(VulkanInstance, &surfaceCreateInfo, nullptr, &windowSurface));
    #else
    auto windowData = GetWaylandWindowData(window);
    SystemAssert(windowData);
    
    VkWaylandSurfaceCreateInfoKHR surfaceCreateInfo = { VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR };
    surfaceCreateInfo.display = windowData->WaylandDisplay;
    surfaceCreateInfo.surface = windowData->WaylandSurface;

    VkSurfaceKHR windowSurface;
    AssertIfFailed(vkCreateWaylandSurfaceKHR(VulkanInstance, &surfaceCreateInfo, nullptr, &windowSurface));
    #endif

    VkBool32 isPresentSupported;
    AssertIfFailed(vkGetPhysicalDeviceSurfaceSupportKHR(graphicsDeviceDataFull->PhysicalDevice, commandQueueData->QueueFamilyIndex, windowSurface, &isPresentSupported));
    SystemAssert(isPresentSupported);

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
    auto refreshRate = windowData->MonitorRefreshRate;

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
    
    // TODO: Extract function
    SystemLogDebugMessage(ElemLogMessageCategory_Graphics, "Create swapchain with size: %dx%d@%f. (%dHz)", windowRenderSize.Width, windowRenderSize.Height, windowRenderSize.UIScale, refreshRate);

    VkSwapchainCreateInfoKHR swapChainCreateInfo = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
    swapChainCreateInfo.surface = windowSurface;
    swapChainCreateInfo.minImageCount = VULKAN_MAX_SWAPCHAIN_BUFFERS;
    swapChainCreateInfo.imageFormat = format;
    swapChainCreateInfo.imageColorSpace = colorSpace;
    swapChainCreateInfo.imageExtent.width = width;
    swapChainCreateInfo.imageExtent.height = height;

    auto swapChain = CreateVulkanSwapChainObject(commandQueueData->GraphicsDevice, windowSurface, &swapChainCreateInfo, nullptr);

    VkFence acquireFence;
    VkFenceCreateInfo fenceCreateInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    AssertIfFailed(vkCreateFence(graphicsDeviceData->Device, &fenceCreateInfo, 0, &acquireFence ));

    uint64_t creationTimestamp;
    // TODO: Put that in a system function
    //QueryPerformanceCounter(&creationTimestamp);

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
        .AspectRatio = (float)width / height,
        .Format = ElemGraphicsFormat_B8G8R8A8_SRGB, // TODO: change that
        .FrameLatency = frameLatency,
        .TargetFPS = targetFPS
    });

    SystemAddDataPoolItemFull(vulkanSwapChainPool, handle, {
        .VulkanFormat = format,
        .CreateInfo = swapChainCreateInfo
    });

    CreateVulkanSwapChainBackBuffers(handle, width, height);

    #ifdef _WIN32
    AddWin32RunLoopHandler(CheckVulkanAvailableSwapChain, handle);
    #elif __linux__
    AddWaylandInitHandler(CheckVulkanAvailableSwapChain, handle);

    auto waylandCallbackParameters = SystemPushStruct<WaylandCallbackParameters>(VulkanGraphicsMemoryArena);
    waylandCallbackParameters->SwapChain = handle;
    waylandCallbackParameters->Window = window;
    waylandCallbackParameters->WaylandSurface = windowData->WaylandSurface;

    RegisterWaylandFrameCallback(waylandCallbackParameters);
    #endif

    return handle;
}

void VulkanFreeSwapChain(ElemSwapChain swapChain)
{
    SystemAssert(swapChain != ELEM_HANDLE_NULL);

    auto swapChainData = GetVulkanSwapChainData(swapChain);
    SystemAssert(swapChainData);

    auto graphicsDeviceData = GetVulkanGraphicsDeviceData(swapChainData->GraphicsDevice);
    SystemAssert(graphicsDeviceData);

    auto fence = CreateVulkanCommandQueueFence(swapChainData->CommandQueue);
    ElemWaitForFenceOnCpu(fence);

    vkDestroyFence(graphicsDeviceData->Device, swapChainData->BackBufferAcquireFence, nullptr);

    for (int i = 0; i < VULKAN_MAX_SWAPCHAIN_BUFFERS; i++)
    {
        auto textureData = GetVulkanGraphicsResourceData(swapChainData->BackBufferTextures[i]);
        SystemAssert(textureData);

        VulkanFreeGraphicsResource(swapChainData->BackBufferTextures[i], nullptr);
    }

    vkDestroySwapchainKHR(graphicsDeviceData->Device, swapChainData->DeviceObject, nullptr);
    vkDestroySurfaceKHR(VulkanInstance, swapChainData->WindowSurface, nullptr);

    SystemRemoveDataPoolItem(vulkanSwapChainPool, swapChain);
}

ElemSwapChainInfo VulkanGetSwapChainInfo(ElemSwapChain swapChain)
{
    SystemAssert(swapChain != ELEM_HANDLE_NULL);

    auto swapChainData = GetVulkanSwapChainData(swapChain);
    SystemAssert(swapChainData);

    return 
    {
        .Width = swapChainData->Width,
        .Height = swapChainData->Height,
        .AspectRatio = swapChainData->AspectRatio,
        .Format = swapChainData->Format
    };
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

    VkPresentIdKHR presentIdInfo = { VK_STRUCTURE_TYPE_PRESENT_ID_KHR };
    presentIdInfo.swapchainCount = 1;
    presentIdInfo.pPresentIds = &swapChainData->PresentId;

    VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &commandQueueData->PresentSemaphore;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapChainData->DeviceObject;
    presentInfo.pImageIndices = &swapChainData->CurrentImageIndex;
    presentInfo.pNext = &presentIdInfo;

    AssertIfFailed(vkQueuePresentKHR(commandQueueData->DeviceObject, &presentInfo));
    
    VulkanResetCommandAllocation(swapChainData->GraphicsDevice);
    VulkanProcessGraphicsResourceDeleteQueue();
    swapChainData->PresentId++;
}

