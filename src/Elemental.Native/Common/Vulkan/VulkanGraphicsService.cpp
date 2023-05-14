#pragma warning(disable: 4191)
#define VK_USE_PLATFORM_WIN32_KHR
#define VOLK_IMPLEMENTATION
#include "volk.h"
#pragma warning(default: 4191)

struct VulkanGraphicsDevice;

#define MAX_VULKAN_GRAPHICS_DEVICES 64
#define MAX_VULKAN_COMMAND_POOLS 64
#define MAX_VULKAN_COMMAND_BUFFERS 64

#include "VulkanCommandQueue.h"
#include "VulkanCommandList.h"
#include "VulkanShader.h"
#include "VulkanTexture.h"
#include "VulkanSwapChain.h"
#include "VulkanGraphicsDevice.h"

GraphicsDiagnostics _vulkanGraphicsDiagnostics;
VkInstance _vulkanInstance = nullptr;
VkDebugReportCallbackEXT _vulkanDebugCallback = nullptr;
uint32_t _vulkanCurrentDeviceInternalId = 0;

void VulkanWaitForFenceOnCpu(Fence fence);

GraphicsDeviceInfo VulkanConstructGraphicsDeviceInfo(VkPhysicalDeviceProperties deviceProperties, VkPhysicalDeviceMemoryProperties deviceMemoryProperties);
VkDeviceQueueCreateInfo VulkanCreateDeviceQueueCreateInfo(uint32_t queueFamilyIndex, uint32_t count);

VulkanCommandPoolItem* VulkanGetCommandPool(VulkanCommandQueue* commandQueue);
void VulkanUpdateCommandPoolFence(VulkanCommandList* commandList, uint64_t fenceValue);
VulkanCommandList* VulkanGetCommandList(VulkanCommandQueue* commandQueue, VulkanCommandPoolItem* commandPoolItem);
Fence VulkanCreateCommandQueueFence(VulkanCommandQueue* commandQueue);

void VulkanCreateSwapChainBackBuffers(VulkanSwapChain* swapChain, int32_t width, int32_t height);

uint64_t VulkanComputeRenderPipelineStateHash(VulkanShader* shader, RenderPassDescriptor* renderPassDescriptor);
VulkanPipelineStateCacheItem* VulkanCreateRenderPipelineState(VulkanShader* shader, RenderPassDescriptor* renderPassDescriptor);

void VulkanTransitionTextureToState(VulkanCommandList* commandList, VulkanTexture* texture, VkImageLayout sourceState, VkImageLayout destinationState, bool isTransfer = false);

static VkBool32 VKAPI_CALL VulkanDebugReportCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char *pLayerPrefix, const char *pMessage, void *pUserData);
static void VulkanDeletePipelineCacheItem(uint64_t key, void* data);

thread_local VulkanDeviceCommandPools CommandPools[MAX_VULKAN_GRAPHICS_DEVICES];

void VulkanInitGraphicsService(GraphicsServiceOptions* options)
{
    // HACK: For the moment, the app will run only if Vulkan SDK 1.3 is installed
    // We need to package the runtime
    // TODO: If the vulkan loader is not here, don't proceed

    _vulkanGraphicsDiagnostics = options->GraphicsDiagnostics;

    AssertIfFailed(volkInitialize());
    assert(volkGetInstanceVersion() >= VK_API_VERSION_1_3);

    VkApplicationInfo appInfo = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
    appInfo.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo createInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
    createInfo.pApplicationInfo = &appInfo;

    auto isSdkInstalled = SystemLoadLibrary("VkLayer_khronos_validation") != nullptr;

    if (options->GraphicsDiagnostics == GraphicsDiagnostics_Debug && isSdkInstalled)
    {
        LogDebugMessage(LogMessageCategory_Graphics, L"Vulkan Debug Mode");

        const char* layers[] =
        {
            "VK_LAYER_KHRONOS_validation"
        };

        createInfo.ppEnabledLayerNames = layers;
        createInfo.enabledLayerCount = ARRAYSIZE(layers);
        
        const char* extensions[] =
        {
            VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
            VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
            VK_KHR_SURFACE_EXTENSION_NAME,
            #ifdef _WINDOWS
            VK_KHR_WIN32_SURFACE_EXTENSION_NAME
            #endif
        };
        
        createInfo.ppEnabledExtensionNames = extensions;
        createInfo.enabledExtensionCount = ARRAYSIZE(extensions);

        VkValidationFeatureEnableEXT enabledValidationFeatures[] =
        {
            VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT,
            VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT
        };

        VkValidationFeaturesEXT validationFeatures = { VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT };
        validationFeatures.enabledValidationFeatureCount = ARRAYSIZE(enabledValidationFeatures);
        validationFeatures.pEnabledValidationFeatures = enabledValidationFeatures;

        createInfo.pNext = &validationFeatures;
        
        AssertIfFailed(vkCreateInstance(&createInfo, nullptr, &_vulkanInstance));
    }

    else
    {
        const char* extensions[] =
        {
            VK_KHR_SURFACE_EXTENSION_NAME,
            #ifdef _WINDOWS
            VK_KHR_WIN32_SURFACE_EXTENSION_NAME
            #endif
        };
        
        createInfo.ppEnabledExtensionNames = extensions;
        createInfo.enabledExtensionCount = ARRAYSIZE(extensions);

        AssertIfFailed(vkCreateInstance(&createInfo, nullptr, &_vulkanInstance));
    }

    volkLoadInstanceOnly(_vulkanInstance);

    if (options->GraphicsDiagnostics == GraphicsDiagnostics_Debug && isSdkInstalled)
    {
        VkDebugReportCallbackCreateInfoEXT debugCreateInfo = { VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT };
        debugCreateInfo.flags = VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | VK_DEBUG_REPORT_ERROR_BIT_EXT;
        debugCreateInfo.pfnCallback = VulkanDebugReportCallback;

        AssertIfFailed(vkCreateDebugReportCallbackEXT(_vulkanInstance, &debugCreateInfo, 0, &_vulkanDebugCallback));
    }
}

void VulkanFreeGraphicsService()
{
    if (_vulkanDebugCallback != nullptr)
    {
        vkDestroyDebugReportCallbackEXT(_vulkanInstance, _vulkanDebugCallback, nullptr);
    }

    if (_vulkanInstance != nullptr)
    {
        vkDestroyInstance(_vulkanInstance, nullptr);
    }
}

void VulkanGetAvailableGraphicsDevices(GraphicsDeviceInfo* graphicsDevices, int* count)
{
    uint32_t deviceCount = 16;
    VkPhysicalDevice devices[16];

    AssertIfFailed(vkEnumeratePhysicalDevices(_vulkanInstance, &deviceCount, nullptr));
    AssertIfFailed(vkEnumeratePhysicalDevices(_vulkanInstance, &deviceCount, devices));

    for (uint32_t i = 0; i < deviceCount; i++)
    {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(devices[i], &deviceProperties);

        VkPhysicalDeviceMemoryProperties deviceMemoryProperties;
        vkGetPhysicalDeviceMemoryProperties(devices[i], &deviceMemoryProperties);

        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceFeatures(devices[i], &deviceFeatures);

        VkPhysicalDevicePresentIdFeaturesKHR presentIdFeatures {};
        presentIdFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_ID_FEATURES_KHR;

        VkPhysicalDeviceMeshShaderFeaturesEXT meshShaderFeatures = {};
        meshShaderFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT;
        meshShaderFeatures.pNext = &presentIdFeatures;

        VkPhysicalDeviceFeatures2 features2 = {};
        features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR;
        features2.pNext = &meshShaderFeatures;

        vkGetPhysicalDeviceFeatures2(devices[i], &features2);

        if (meshShaderFeatures.meshShader && meshShaderFeatures.taskShader && presentIdFeatures.presentId)
        {
            graphicsDevices[(*count)++] = VulkanConstructGraphicsDeviceInfo(deviceProperties, deviceMemoryProperties);
        }
    }
}

void* VulkanCreateGraphicsDevice(GraphicsDeviceOptions* options)
{
    uint32_t deviceCount = 16;
    VkPhysicalDevice devices[16];
    VkPhysicalDevice physicalDevice = {};
    VkPhysicalDeviceProperties deviceProperties = {};
    VkPhysicalDeviceMemoryProperties deviceMemoryProperties {};
    bool foundDevice = false;

    AssertIfFailed(vkEnumeratePhysicalDevices(_vulkanInstance, &deviceCount, nullptr));
    AssertIfFailed(vkEnumeratePhysicalDevices(_vulkanInstance, &deviceCount, devices));

    for (uint32_t i = 0; i < deviceCount; i++)
    {
        vkGetPhysicalDeviceProperties(devices[i], &deviceProperties);
        vkGetPhysicalDeviceMemoryProperties(devices[i], &deviceMemoryProperties);

        if (deviceProperties.deviceID == options->DeviceId)
        {
            physicalDevice = devices[i];
            foundDevice = true;
            break;
        }
    }

    if (!foundDevice)
    {
        return nullptr;
    }
    
    auto graphicsDevice = new VulkanGraphicsDevice();

    uint32_t queueFamilyCount = 0;
    VkQueueFamilyProperties queueFamilies[32];
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies);

    VkDeviceQueueCreateInfo queueCreateInfos[3];

    for (uint32_t i = 0; i < queueFamilyCount; i++)
    {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT && graphicsDevice->RenderCommandQueueFamilyIndex == UINT32_MAX)
        {
            graphicsDevice->RenderCommandQueueFamilyIndex = i;
            queueCreateInfos[i] = VulkanCreateDeviceQueueCreateInfo(i, 2);
        }

        else if (queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT && graphicsDevice->ComputeCommandQueueFamilyIndex == UINT32_MAX)
        {
            graphicsDevice->ComputeCommandQueueFamilyIndex = i;
            queueCreateInfos[i] = VulkanCreateDeviceQueueCreateInfo(i, 1);
        }

        else if (queueFamilies[i].queueFlags & VK_QUEUE_TRANSFER_BIT && graphicsDevice->CopyCommandQueueFamilyIndex == UINT32_MAX)
        {
            graphicsDevice->CopyCommandQueueFamilyIndex = i;
            queueCreateInfos[i] = VulkanCreateDeviceQueueCreateInfo(i, 1);
        }
    }

    graphicsDevice->InternalId = InterlockedIncrement(&_vulkanCurrentDeviceInternalId) - 1;
    graphicsDevice->PhysicalDevice = physicalDevice;
    graphicsDevice->DeviceProperties = deviceProperties;
    graphicsDevice->DeviceMemoryProperties = deviceMemoryProperties;

    VkDeviceCreateInfo createInfo = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
    createInfo.queueCreateInfoCount = 3;
    createInfo.pQueueCreateInfos = queueCreateInfos;

    const char* extensions[] =
    {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_PRESENT_ID_EXTENSION_NAME,
        VK_KHR_PRESENT_WAIT_EXTENSION_NAME,
        VK_EXT_MESH_SHADER_EXTENSION_NAME
    };

    createInfo.ppEnabledExtensionNames = extensions;
    createInfo.enabledExtensionCount = ARRAYSIZE(extensions);

    VkPhysicalDeviceFeatures2 features = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
    features.features.multiDrawIndirect = true;
    features.features.pipelineStatisticsQuery = true;
    features.features.shaderInt16 = true;
    features.features.shaderInt64 = true;

    VkPhysicalDeviceVulkan12Features features12 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
    features12.timelineSemaphore = true;
    features12.runtimeDescriptorArray = true;
    features12.descriptorIndexing = true;
    features12.descriptorBindingVariableDescriptorCount = true;
    features12.descriptorBindingPartiallyBound = true;
    features12.descriptorBindingSampledImageUpdateAfterBind = true;
    features12.descriptorBindingStorageBufferUpdateAfterBind = true;
    features12.descriptorBindingStorageImageUpdateAfterBind = true;
    features12.shaderSampledImageArrayNonUniformIndexing = true;
    features12.separateDepthStencilLayouts = true;
    features12.hostQueryReset = true;
    features12.shaderInt8 = true;

    if (_vulkanGraphicsDiagnostics == GraphicsDiagnostics_Debug)
    {
        features12.bufferDeviceAddressCaptureReplay = true;
    }

    VkPhysicalDeviceVulkan13Features features13 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
    features13.maintenance4 = true;
    features13.synchronization2 = true;
    features13.dynamicRendering = true;

    VkPhysicalDeviceMeshShaderFeaturesEXT meshFeatures = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT };
    meshFeatures.meshShader = true;
    meshFeatures.taskShader = true;
    meshFeatures.meshShaderQueries = true;

    VkPhysicalDevicePresentIdFeaturesKHR presentIdFeatures { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_ID_FEATURES_KHR };
    presentIdFeatures.presentId = true;
    
    VkPhysicalDevicePresentWaitFeaturesKHR presentWaitFeatures { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_WAIT_FEATURES_KHR };
    presentWaitFeatures.presentWait = true;

    createInfo.pNext = &features;
    features.pNext = &features12;
    features12.pNext = &features13;
    features13.pNext = &presentIdFeatures;
    presentIdFeatures.pNext = &presentWaitFeatures;
    presentWaitFeatures.pNext = &meshFeatures;

    if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &(graphicsDevice->Device)) != VK_SUCCESS)
    {
        return nullptr;
    }

    volkLoadDevice(graphicsDevice->Device);

    for (int i = 0; i < 3; i++)
    {
        delete queueCreateInfos[i].pQueuePriorities;
    }

    return graphicsDevice;
}

void VulkanFreeGraphicsDevice(void* graphicsDevicePointer)
{
    auto graphicsDevice = (VulkanGraphicsDevice*)graphicsDevicePointer;
    VulkanCommandPoolItem* item;

    graphicsDevice->PipelineStates.EnumerateEntries(VulkanDeletePipelineCacheItem);

    for (uint32_t i = 0; i < MAX_VULKAN_COMMAND_POOLS; i++)
    {
        graphicsDevice->DirectCommandPool.GetCurrentItemPointerAndMove(&item);

        if (item->CommandPool != nullptr)
        {
            vkDestroyCommandPool(graphicsDevice->Device, item->CommandPool, nullptr);
        }

        graphicsDevice->ComputeCommandPool.GetCurrentItemPointerAndMove(&item);

        if (item->CommandPool != nullptr)
        {
            vkDestroyCommandPool(graphicsDevice->Device, item->CommandPool, nullptr);
        }

        graphicsDevice->CopyCommandPool.GetCurrentItemPointerAndMove(&item);

        if (item->CommandPool != nullptr)
        {
            vkDestroyCommandPool(graphicsDevice->Device, item->CommandPool, nullptr);
        }
    }
    
    if (graphicsDevice->Device != nullptr)
    {
        vkDestroyDevice(graphicsDevice->Device, nullptr);
        delete graphicsDevice;
    }
}

GraphicsDeviceInfo VulkanGetGraphicsDeviceInfo(void* graphicsDevicePointer)
{
    auto graphicsDevice = (VulkanGraphicsDevice*)graphicsDevicePointer;
    return VulkanConstructGraphicsDeviceInfo(graphicsDevice->DeviceProperties, graphicsDevice->DeviceMemoryProperties);
}

void* VulkanCreateCommandQueue(void* graphicsDevicePointer, CommandQueueType type)
{
    auto graphicsDevice = (VulkanGraphicsDevice*)graphicsDevicePointer;

    auto commandQueue = new VulkanCommandQueue(graphicsDevice);
    commandQueue->CommandQueueType = type;

    auto queueFamilyIndex = graphicsDevice->RenderCommandQueueFamilyIndex;

    if (type == CommandQueueType_Compute)
    {
        queueFamilyIndex = graphicsDevice->ComputeCommandQueueFamilyIndex;
    }

    else if (type == CommandQueueType_Copy)
    {
        queueFamilyIndex = graphicsDevice->CopyCommandQueueFamilyIndex;
    }

    commandQueue->FamilyIndex = queueFamilyIndex;

    // TODO: Check count of VK Queues already created for this device
    vkGetDeviceQueue(graphicsDevice->Device, queueFamilyIndex, 0, &commandQueue->DeviceObject);

    VkSemaphoreTypeCreateInfo timelineCreateInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO };
    timelineCreateInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
    timelineCreateInfo.initialValue = 0;

    VkSemaphoreCreateInfo createInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    createInfo.pNext = &timelineCreateInfo;

    AssertIfFailed(vkCreateSemaphore(graphicsDevice->Device, &createInfo, NULL, &commandQueue->TimelineSemaphore));
    return commandQueue;
}

void VulkanFreeCommandQueue(void* commandQueuePointer)
{
    auto commandQueue = (VulkanCommandQueue*)commandQueuePointer;
    
    auto fence = VulkanCreateCommandQueueFence(commandQueue);
    VulkanWaitForFenceOnCpu(fence);
    
    vkDestroySemaphore(commandQueue->GraphicsDevice->Device, commandQueue->TimelineSemaphore, nullptr);
    delete commandQueue;
}

void VulkanSetCommandQueueLabel(void* commandQueuePointer, uint8_t* label)
{
    auto commandQueue = (VulkanCommandQueue*)commandQueuePointer;
    
    VkDebugUtilsObjectNameInfoEXT nameInfo = { VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT };
    nameInfo.objectType = VK_OBJECT_TYPE_QUEUE;
    nameInfo.objectHandle = (uint64_t)commandQueue->DeviceObject;
    nameInfo.pObjectName = (char*)label;

    AssertIfFailed(vkSetDebugUtilsObjectNameEXT(commandQueue->GraphicsDevice->Device, &nameInfo)); 
}

void* VulkanCreateCommandList(void* commandQueuePointer)
{
    auto commandQueue = (VulkanCommandQueue*)commandQueuePointer;

    auto commandPool = VulkanGetCommandPool(commandQueue);
    auto commandList = VulkanGetCommandList(commandQueue, commandPool);

    VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    AssertIfFailed(vkBeginCommandBuffer(commandList->DeviceObject, &beginInfo));

    return commandList;
}

void VulkanFreeCommandList(void* commandListPointer)
{
    auto commandList = (VulkanCommandList*)commandListPointer;
    
    if (!commandList->IsFromCommandPool)
    {
        delete commandList;
    }
}

void VulkanSetCommandListLabel(void* commandListPointer, uint8_t* label)
{
    auto commandList = (VulkanCommandList*)commandListPointer;

    VkDebugUtilsObjectNameInfoEXT nameInfo = { VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT };
    nameInfo.objectType = VK_OBJECT_TYPE_COMMAND_BUFFER;
    nameInfo.objectHandle = (uint64_t)commandList->DeviceObject;
    nameInfo.pObjectName = (char*)label;

    AssertIfFailed(vkSetDebugUtilsObjectNameEXT(commandList->GraphicsDevice->Device, &nameInfo));
}

void VulkanCommitCommandList(void* commandListPointer)
{
    auto commandList = (VulkanCommandList*)commandListPointer;
    AssertIfFailed(vkEndCommandBuffer(commandList->DeviceObject));
}

Fence VulkanExecuteCommandLists(void* commandQueuePointer, void** commandLists, int32_t commandListCount, Fence* fencesToWait, int32_t fenceToWaitCount)
{
    auto commandQueue = (VulkanCommandQueue*)commandQueuePointer;

    VkPipelineStageFlags submitStageMasks[MAX_VULKAN_COMMAND_BUFFERS];
    VkSemaphore waitSemaphores[MAX_VULKAN_COMMAND_BUFFERS];
    uint64_t waitSemaphoreValues[MAX_VULKAN_COMMAND_BUFFERS];

    for (int32_t i = 0; i < fenceToWaitCount; i++)
    {
        auto fenceToWait = fencesToWait[i];
        auto commandQueueToWait = (VulkanCommandQueue*)fenceToWait.CommandQueuePointer;

        waitSemaphores[i] = commandQueueToWait->TimelineSemaphore;
        waitSemaphoreValues[i] = commandQueueToWait->FenceValue;
        submitStageMasks[i] = (commandQueue->CommandQueueType == CommandQueueType_Compute) ? VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT : VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
    }

    VkCommandBuffer vulkanCommandBuffers[MAX_VULKAN_COMMAND_BUFFERS];

    for (int32_t i = 0; i < commandListCount; i++)
    {
        auto vulkanCommandList = (VulkanCommandList*)commandLists[i];
        vulkanCommandBuffers[i] = vulkanCommandList->DeviceObject;
    }
    
    auto fenceValue = InterlockedIncrement(&commandQueue->FenceValue);

    VkTimelineSemaphoreSubmitInfo timelineInfo = { VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO };
    timelineInfo.waitSemaphoreValueCount = (uint32_t)fenceToWaitCount;
    timelineInfo.pWaitSemaphoreValues = (fenceToWaitCount > 0) ? waitSemaphoreValues : nullptr;
    timelineInfo.signalSemaphoreValueCount = 1;
    timelineInfo.pSignalSemaphoreValues = &fenceValue;

    VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submitInfo.pNext = &timelineInfo;
    submitInfo.waitSemaphoreCount = (uint32_t)fenceToWaitCount;
    submitInfo.pWaitSemaphores = (fenceToWaitCount > 0) ? waitSemaphores : nullptr;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &commandQueue->TimelineSemaphore;
    submitInfo.commandBufferCount = (uint32_t)commandListCount;
    submitInfo.pCommandBuffers = vulkanCommandBuffers;
    submitInfo.pWaitDstStageMask = submitStageMasks;

    AssertIfFailed(vkQueueSubmit(commandQueue->DeviceObject, 1, &submitInfo, VK_NULL_HANDLE));

    for (int32_t i = 0; i < commandListCount; i++)
    {
        auto vulkanCommandList = (VulkanCommandList*)commandLists[i];

        if (vulkanCommandList->IsFromCommandPool)
        {
            VulkanUpdateCommandPoolFence(vulkanCommandList, fenceValue);
        }
    }

    auto fence = Fence();
    fence.CommandQueuePointer = commandQueue;
    fence.FenceValue = fenceValue;

    return fence;
}

void VulkanWaitForFenceOnCpu(Fence fence)
{
    auto commandQueueToWait = (VulkanCommandQueue*)fence.CommandQueuePointer;

    if (fence.FenceValue > commandQueueToWait->LastCompletedFenceValue) 
    {
        uint64_t semaphoreValue;
        vkGetSemaphoreCounterValue(commandQueueToWait->GraphicsDevice->Device, commandQueueToWait->TimelineSemaphore, &semaphoreValue);

        commandQueueToWait->LastCompletedFenceValue = max(commandQueueToWait->LastCompletedFenceValue, semaphoreValue);
    }

    if (fence.FenceValue > commandQueueToWait->LastCompletedFenceValue)
    {
        LogDebugMessage(LogMessageCategory_Graphics, L"Wait for fence on CPU...");

        VkSemaphoreWaitInfo waitInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO };
        waitInfo.semaphoreCount = 1;
        waitInfo.pSemaphores = &commandQueueToWait->TimelineSemaphore;
        waitInfo.pValues = &fence.FenceValue;

        AssertIfFailed(vkWaitSemaphores(commandQueueToWait->GraphicsDevice->Device, &waitInfo, UINT64_MAX));
    }
}

void VulkanResetCommandAllocation(void* graphicsDevicePointer)
{
    auto graphicsDevice = (VulkanGraphicsDevice*)graphicsDevicePointer;
    graphicsDevice->CommandPoolGeneration++;
}

void VulkanFreeTexture(void* texturePointer)
{
    auto texture = (VulkanTexture*)texturePointer;

    if (!texture->IsPresentTexture)
    {
        auto graphicsDevice = texture->GraphicsDevice;

        vkDestroyImageView(graphicsDevice->Device, texture->ImageView, nullptr);
        vkDestroyImage(graphicsDevice->Device, texture->DeviceObject, nullptr);

        delete texture;    
    }
}

void* VulkanCreateSwapChain(void* windowPointer, void* commandQueuePointer, SwapChainOptions* options)
{
    auto commandQueue = (VulkanCommandQueue*)commandQueuePointer;
    auto graphicsDevice = commandQueue->GraphicsDevice;

    auto swapChain = new VulkanSwapChain(graphicsDevice);
    swapChain->CommandQueue = commandQueue;
    swapChain->Format = options->Format;
    swapChain->CurrentImageIndex = 0;
    swapChain->MaximumFrameLatency = options->MaximumFrameLatency;

    #ifdef _WINDOWS
    auto window = (Win32Window*)windowPointer;
    VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = { VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
    surfaceCreateInfo.hinstance = GetModuleHandle(nullptr);
    surfaceCreateInfo.hwnd = window->WindowHandle;

    AssertIfFailed(vkCreateWin32SurfaceKHR(_vulkanInstance, &surfaceCreateInfo, nullptr, &swapChain->WindowSurface));
    #endif

    VkBool32 isPresentSupported;
    AssertIfFailed(vkGetPhysicalDeviceSurfaceSupportKHR(graphicsDevice->PhysicalDevice, swapChain->CommandQueue->FamilyIndex, swapChain->WindowSurface, &isPresentSupported));
    assert(isPresentSupported == 1);

    VkSurfaceCapabilitiesKHR surfaceCapabilities = {};
    AssertIfFailed(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(graphicsDevice->PhysicalDevice, swapChain->WindowSurface, &surfaceCapabilities));

    uint32_t surfaceFormatCount = 0;
    VkSurfaceFormatKHR surfaceFormats[16];
    vkGetPhysicalDeviceSurfaceFormatsKHR(graphicsDevice->PhysicalDevice, swapChain->WindowSurface, &surfaceFormatCount, nullptr);
    vkGetPhysicalDeviceSurfaceFormatsKHR(graphicsDevice->PhysicalDevice, swapChain->WindowSurface, &surfaceFormatCount, surfaceFormats);
    assert(surfaceFormatCount > 0);

    VkSwapchainCreateInfoKHR swapChainCreateInfo = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
    swapChainCreateInfo.surface = swapChain->WindowSurface;
    swapChainCreateInfo.minImageCount = 3;
    swapChainCreateInfo.imageFormat = options->Format == SwapChainFormat_HighDynamicRange ? VK_FORMAT_R16G16B16A16_SFLOAT : VK_FORMAT_B8G8R8A8_SRGB;
    swapChainCreateInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    swapChainCreateInfo.imageArrayLayers = 1;
    swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapChainCreateInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    swapChainCreateInfo.preTransform = surfaceCapabilities.currentTransform;
    swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapChainCreateInfo.flags = 0;
    
    if (options->Width == 0 || options->Height == 0)
    {
        auto windowRenderSize = Native_GetWindowRenderSize(window);
        swapChainCreateInfo.imageExtent.width = windowRenderSize.Width;
        swapChainCreateInfo.imageExtent.height = windowRenderSize.Height;
    }
    else
    {
        swapChainCreateInfo.imageExtent.width = options->Width;
        swapChainCreateInfo.imageExtent.height = options->Height;
    }

    swapChain->CreateInfo = swapChainCreateInfo;
    AssertIfFailed(vkCreateSwapchainKHR(graphicsDevice->Device, &swapChainCreateInfo, nullptr, &swapChain->DeviceObject));

    VulkanCreateSwapChainBackBuffers(swapChain, (int32_t)swapChainCreateInfo.imageExtent.width, (int32_t)swapChainCreateInfo.imageExtent.height);

    VkFenceCreateInfo fenceCreateInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    AssertIfFailed(vkCreateFence(graphicsDevice->Device, &fenceCreateInfo, 0, &swapChain->BackBufferAcquireFence ));

    return swapChain;
}

void VulkanFreeSwapChain(void* swapChainPointer)
{
    auto swapChain = (VulkanSwapChain*)swapChainPointer;
    auto graphicsDevice = swapChain->GraphicsDevice;

    vkDestroyFence(graphicsDevice->Device, swapChain->BackBufferAcquireFence, nullptr);

    for (int i = 0; i < 3; i++)
    {
        vkDestroyImageView(graphicsDevice->Device, swapChain->BackBufferTextures[i]->ImageView, nullptr);
        delete swapChain->BackBufferTextures[i];
    }

    vkDestroySwapchainKHR(swapChain->GraphicsDevice->Device, swapChain->DeviceObject, nullptr);
    vkDestroySurfaceKHR(_vulkanInstance, swapChain->WindowSurface, nullptr);

    delete swapChain;
}

void VulkanResizeSwapChain(void* swapChainPointer, int width, int height)
{
    if (width == 0 || height == 0)
    {
        return;
    }

    auto swapChain = (VulkanSwapChain*)swapChainPointer;
    auto graphicsDevice = swapChain->GraphicsDevice;

    auto fence = VulkanCreateCommandQueueFence(swapChain->CommandQueue);
    VulkanWaitForFenceOnCpu(fence);

    swapChain->CurrentPresentId = 0;
    auto oldSwapChain = swapChain->DeviceObject;

    auto swapChainCreateInfo = swapChain->CreateInfo;
    swapChainCreateInfo.oldSwapchain = oldSwapChain;
    swapChainCreateInfo.imageExtent.width = (uint32_t)width;
    swapChainCreateInfo.imageExtent.height = (uint32_t)height;
    
    AssertIfFailed(vkCreateSwapchainKHR(graphicsDevice->Device, &swapChainCreateInfo, nullptr, &swapChain->DeviceObject));

    for (int32_t i = 0; i < 3; i++)
    {
        auto texture = swapChain->BackBufferTextures[i];
        vkDestroyImageView(graphicsDevice->Device, texture->ImageView, nullptr);
    }

    vkDestroySwapchainKHR(graphicsDevice->Device, oldSwapChain, nullptr);
    
    VulkanCreateSwapChainBackBuffers(swapChain, (int32_t)swapChainCreateInfo.imageExtent.width, (int32_t)swapChainCreateInfo.imageExtent.height);
}

void* VulkanGetSwapChainBackBufferTexture(void* swapChainPointer)
{
    auto swapChain = (VulkanSwapChain*)swapChainPointer;
    return swapChain->BackBufferTextures[swapChain->CurrentImageIndex];
}

void VulkanPresentSwapChain(void* swapChainPointer)
{
    // TODO: Wait for the correct timeline semaphore value?
    // Or just issue a barrier because the final buffer rendering and the present is done on the same queue
    auto swapChain = (VulkanSwapChain*)swapChainPointer;

    // HACK: We should set the release semaphore to the latest command buffer
    // Otherwise, the results maybe undefined?

    VkPresentIdKHR presentIdInfo = { VK_STRUCTURE_TYPE_PRESENT_ID_KHR };
    presentIdInfo.swapchainCount = 1;
    presentIdInfo.pPresentIds = &swapChain->CurrentPresentId;

    VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
    presentInfo.waitSemaphoreCount = 0;
    presentInfo.pWaitSemaphores = nullptr;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapChain->DeviceObject;
    presentInfo.pImageIndices = &swapChain->CurrentImageIndex;
    presentInfo.pNext = &presentIdInfo;

    AssertIfFailed(vkQueuePresentKHR(swapChain->CommandQueue->DeviceObject, &presentInfo));
    
    VulkanResetCommandAllocation(swapChain->GraphicsDevice);
    swapChain->CurrentPresentId++;
}

void VulkanWaitForSwapChainOnCpu(void* swapChainPointer)
{
    // BUG: There is a high GPU usage when the app is not active
    auto swapChain = (VulkanSwapChain*)swapChainPointer;
    auto graphicsDevice = swapChain->GraphicsDevice;

    // BUG: This method is not working correctly for now because present is blocking the CPU thread
    if (swapChain->CurrentPresentId >= swapChain->MaximumFrameLatency)
    {
        auto presentId = swapChain->CurrentPresentId - swapChain->MaximumFrameLatency;
        AssertIfFailed(vkWaitForPresentKHR(graphicsDevice->Device, swapChain->DeviceObject, presentId, UINT64_MAX));
    }

    // TODO: Do we really need to have that? because we have the present ID now.
    AssertIfFailed(vkAcquireNextImageKHR(swapChain->GraphicsDevice->Device, swapChain->DeviceObject, UINT64_MAX, VK_NULL_HANDLE, swapChain->BackBufferAcquireFence, &swapChain->CurrentImageIndex));
    vkWaitForFences(swapChain->GraphicsDevice->Device, 1, &swapChain->BackBufferAcquireFence, true, UINT64_MAX);
    vkResetFences(swapChain->GraphicsDevice->Device, 1, &swapChain->BackBufferAcquireFence);
}

void* VulkanCreateShader(void* graphicsDevicePointer, ShaderPart* shaderParts, int32_t shaderPartCount)
{
    auto graphicsDevice = (VulkanGraphicsDevice*)graphicsDevicePointer;
    auto shader = new VulkanShader(graphicsDevice);

    auto pushConstantCount = 0u;
    
    for (int32_t i = 0; i < shaderPartCount; i++)
    {
        auto shaderPart = shaderParts[i];

        if (pushConstantCount == 0)
        {
            for (uint32_t j = 0; j < shaderPart.MetaDataCount; j++)
            {
                auto metaData = shaderPart.MetaDataPointer[j];

                if (metaData.Type == ShaderMetaDataType_PushConstantsCount)
                {
                    pushConstantCount = metaData.Value;
                    break;
                }
            }
        }

        VkShaderModuleCreateInfo createInfo = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        createInfo.codeSize = (size_t)shaderPart.DataCount;
        createInfo.pCode = reinterpret_cast<const uint32_t*>(shaderPart.DataPointer);

        VkShaderModule shaderModule = nullptr;
        AssertIfFailed(vkCreateShaderModule(graphicsDevice->Device, &createInfo, 0, &shaderModule));

        switch (shaderPart.Stage)
        {
        case ShaderStage_AmplificationShader:
            shader->AmplificationShader = shaderModule;
            memcpy(shader->AmplificationShaderEntryPoint, shaderPart.EntryPoint, strlen((char*)shaderPart.EntryPoint) + 1);
            break;

        case ShaderStage_MeshShader:
            shader->MeshShader = shaderModule;
            memcpy(shader->MeshShaderEntryPoint, shaderPart.EntryPoint, strlen((char*)shaderPart.EntryPoint) + 1);
            break;

        case ShaderStage_PixelShader:
            shader->PixelShader = shaderModule;
            memcpy(shader->PixelShaderEntryPoint, shaderPart.EntryPoint, strlen((char*)shaderPart.EntryPoint) + 1);
            break;
        }
    }

	VkPipelineLayoutCreateInfo layoutCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	layoutCreateInfo.pSetLayouts = nullptr;
	layoutCreateInfo.setLayoutCount = 0;

    VkPushConstantRange push_constant;
    push_constant.offset = 0;
    push_constant.size = pushConstantCount * 4;
    push_constant.stageFlags = VK_SHADER_STAGE_ALL;

    layoutCreateInfo.pPushConstantRanges = &push_constant;
    layoutCreateInfo.pushConstantRangeCount = 1;

	AssertIfFailed(vkCreatePipelineLayout(graphicsDevice->Device, &layoutCreateInfo, 0, &shader->PipelineLayout));

    return shader;
}

void VulkanFreeShader(void* shaderPointer)
{
    auto shader = (VulkanShader*)shaderPointer;
    auto graphicsDevice = (VulkanGraphicsDevice*)shader->GraphicsDevice;

    if (shader->PipelineLayout != nullptr)
    {
        vkDestroyPipelineLayout(graphicsDevice->Device, shader->PipelineLayout, nullptr);
    }

    if (shader->AmplificationShader != nullptr)
    {
        vkDestroyShaderModule(graphicsDevice->Device, shader->AmplificationShader, nullptr);
    }

    if (shader->MeshShader != nullptr)
    {
        vkDestroyShaderModule(graphicsDevice->Device, shader->MeshShader, nullptr);
    }

    if (shader->PixelShader != nullptr)
    {
        vkDestroyShaderModule(graphicsDevice->Device, shader->PixelShader, nullptr);
    }

    delete shader;
}

void VulkanBeginRenderPass(void* commandListPointer, RenderPassDescriptor* renderPassDescriptor)
{
    auto commandList = (VulkanCommandList*)commandListPointer;

    commandList->CurrentRenderPassDescriptor = *renderPassDescriptor;
    commandList->IsRenderPassActive = true;

    if (renderPassDescriptor->RenderTarget0.HasValue)
    {
        auto texture = (VulkanTexture*)commandList->CurrentRenderPassDescriptor.RenderTarget0.Value.TexturePointer;
        auto clearColor = renderPassDescriptor->RenderTarget0.Value.ClearColor.Value;
        VulkanTransitionTextureToState(commandList, texture, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        VkRenderingAttachmentInfo colorAttachment = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
        colorAttachment.imageView = texture->ImageView;
        colorAttachment.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
        colorAttachment.loadOp = !renderPassDescriptor->RenderTarget0.Value.ClearColor.HasValue ? VK_ATTACHMENT_LOAD_OP_LOAD : VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.clearValue.color = { {clearColor.X, clearColor.Y, clearColor.Z, 1.0f} };

        VkRenderingInfo passInfo = { VK_STRUCTURE_TYPE_RENDERING_INFO };
        passInfo.renderArea.extent.width = texture->Width;
        passInfo.renderArea.extent.height = texture->Height;
        passInfo.layerCount = 1;
        passInfo.colorAttachmentCount = 1;
        passInfo.pColorAttachments = &colorAttachment;

        vkCmdBeginRendering(commandList->DeviceObject, &passInfo);

        VkViewport viewport = { 0, float(texture->Height), float(texture->Width), -float(texture->Height), 0, 1 };
        VkRect2D scissor = { {0, 0}, {uint32_t(texture->Width), uint32_t(texture->Height)} };

        vkCmdSetViewport(commandList->DeviceObject, 0, 1, &viewport);
        vkCmdSetScissor(commandList->DeviceObject, 0, 1, &scissor);
    }
}
    
void VulkanEndRenderPass(void* commandListPointer)
{
    auto commandList = (VulkanCommandList*)commandListPointer;
    
    if (commandList->CurrentRenderPassDescriptor.RenderTarget0.HasValue)
    {
        vkCmdEndRendering(commandList->DeviceObject);

        auto texture = (VulkanTexture*)commandList->CurrentRenderPassDescriptor.RenderTarget0.Value.TexturePointer;

        if (texture->IsPresentTexture)
        {
            VulkanTransitionTextureToState(commandList, texture, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
        }
    }
    
    commandList->CurrentRenderPassDescriptor = {};
    commandList->IsRenderPassActive = false;
}

void VulkanSetShader(void* commandListPointer, void* shaderPointer)
{
    auto commandList = (VulkanCommandList*)commandListPointer;
    auto graphicsDevice = commandList->GraphicsDevice;
    auto shader = (VulkanShader*)shaderPointer;

    if (commandList->IsRenderPassActive)
    {
        // TODO: Hash the parameters
        // TODO: Async compilation with mutlithread support. (Reserve a slot in the cache, and return the pipelinestate cache object)
        // TODO: Have a separate CompileShader function that will launch the async work.
        // TODO: Have a separate GetShaderStatus method
        // TODO: Block for this method, because it means the user wants to use the shader and wants to wait on purpose

        auto renderPassDescriptor = &commandList->CurrentRenderPassDescriptor;
        auto hash = VulkanComputeRenderPipelineStateHash(shader, renderPassDescriptor);

        // TODO: This is not thread-safe!
        // TODO: We should have a kind of GetOrAdd method 
        if (!graphicsDevice->PipelineStates.ContainsKey(hash))
        {
            LogDebugMessage(LogMessageCategory_Graphics, L"Create PipelineState for shader %llu...", hash);
            auto pipelineStateCacheItem = VulkanCreateRenderPipelineState(shader, &commandList->CurrentRenderPassDescriptor);

            graphicsDevice->PipelineStates.Add(hash, pipelineStateCacheItem);
        }

        auto pipelineState = graphicsDevice->PipelineStates[hash];
        assert(pipelineState->PipelineState != nullptr);

        vkCmdBindPipeline(commandList->DeviceObject, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineState->PipelineState);
        commandList->CurrentPipelineState = pipelineState;
        commandList->CurrentShader = shader;
    }
}

void VulkanSetShaderConstants(void* commandListPointer, uint32_t slot, void* constantValues, int32_t constantValueCount)
{
    auto commandList = (VulkanCommandList*)commandListPointer;
    assert(commandList->CurrentShader != nullptr);

    // TODO: There seems that there was a memory leak in the old engine. Check that again!
    vkCmdPushConstants(commandList->DeviceObject, commandList->CurrentShader->PipelineLayout, VK_SHADER_STAGE_ALL, 0, (uint32_t)constantValueCount, constantValues);
}

void VulkanDispatchMesh(void* commandListPointer, uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ)
{
    auto commandList = (VulkanCommandList*)commandListPointer;

    if (commandList->IsRenderPassActive)
    {
        vkCmdDrawMeshTasksEXT(commandList->DeviceObject, threadGroupCountX, threadGroupCountY, threadGroupCountZ);
    }
}
   
GraphicsDeviceInfo VulkanConstructGraphicsDeviceInfo(VkPhysicalDeviceProperties deviceProperties, VkPhysicalDeviceMemoryProperties deviceMemoryProperties)
{
    // TODO: Avoid std::wstring
    auto result = GraphicsDeviceInfo();
    wcscpy_s(result.DeviceName, std::wstring(deviceProperties.deviceName, deviceProperties.deviceName + strlen(deviceProperties.deviceName)).c_str());
    result.GraphicsApi = GraphicsApi_Vulkan;
    result.DeviceId = deviceProperties.deviceID;
    result.AvailableMemory = deviceMemoryProperties.memoryHeaps[0].size;

    return result;
}

VkDeviceQueueCreateInfo VulkanCreateDeviceQueueCreateInfo(uint32_t queueFamilyIndex, uint32_t count)
{
    auto queuePriorities = new float[count];

    for (uint32_t i = 0; i < count; i++)
    {
        queuePriorities[i] = 1.0f;
    }

    VkDeviceQueueCreateInfo queueCreateInfo = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
    queueCreateInfo.pQueuePriorities = queuePriorities;
    queueCreateInfo.queueCount = count;
    queueCreateInfo.queueFamilyIndex = queueFamilyIndex;

    return queueCreateInfo;
}
    
VulkanCommandPoolItem* VulkanGetCommandPool(VulkanCommandQueue* commandQueue)
{
    auto graphicsDevice = commandQueue->GraphicsDevice;

    VulkanDeviceCommandPools& commandPoolsCache = CommandPools[graphicsDevice->InternalId];

    if (commandPoolsCache.Generation != graphicsDevice->CommandPoolGeneration)
    {
        commandPoolsCache.Reset(graphicsDevice->CommandPoolGeneration);
    }

    auto& commandPool = graphicsDevice->DirectCommandPool;
    auto commandPoolItemPointer = &commandPoolsCache.DirectCommandPool;
    
    if (commandQueue->CommandQueueType == CommandQueueType_Copy)
    {
        commandPool = graphicsDevice->CopyCommandPool;
        commandPoolItemPointer = &commandPoolsCache.CopyCommandPool;
    }
    else if (commandQueue->CommandQueueType == CommandQueueType_Compute)
    {
        commandPool = graphicsDevice->ComputeCommandPool;
        commandPoolItemPointer = &commandPoolsCache.ComputeCommandPool;
    }

    if (*commandPoolItemPointer == nullptr)
    {
        VulkanCommandPoolItem* commandPoolItem;
        commandPool.GetCurrentItemPointerAndMove(&commandPoolItem);

        if (commandPoolItem->CommandPool == nullptr)
        {
            VkCommandPoolCreateInfo createInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
            createInfo.flags = 0;
            createInfo.queueFamilyIndex = commandQueue->FamilyIndex;

            AssertIfFailed(vkCreateCommandPool(graphicsDevice->Device, &createInfo, 0, &commandPoolItem->CommandPool));
        }
        else
        {
            assert(commandPoolItem->IsInUse == false);

            if (commandPoolItem->Fence.FenceValue > 0)
            {
                VulkanWaitForFenceOnCpu(commandPoolItem->Fence);
            }

            AssertIfFailed(vkResetCommandPool(graphicsDevice->Device, commandPoolItem->CommandPool, 0));
            commandPoolItem->CurrentCommandListIndex = 0;
            commandPoolItem->IsInUse = true;
        }

        *commandPoolItemPointer = commandPoolItem;
    }

    return (*commandPoolItemPointer);
}
    
void VulkanUpdateCommandPoolFence(VulkanCommandList* commandList, uint64_t fenceValue)
{
    auto fence = Fence();
    fence.CommandQueuePointer = commandList->CommandQueue;
    fence.FenceValue = fenceValue;

    commandList->CommandPoolItem->Fence = fence;
    commandList->CommandPoolItem->IsInUse = false;
}

VulkanCommandList* VulkanGetCommandList(VulkanCommandQueue* commandQueue, VulkanCommandPoolItem* commandPoolItem)
{
    auto graphicsDevice = commandQueue->GraphicsDevice;

    VulkanCommandList** commandListArrayPointer = nullptr;
    VulkanCommandList* commandList = nullptr;
    auto isFromCommandPoolItem = false;

    if (commandPoolItem->CurrentCommandListIndex < MAX_VULKAN_COMMAND_BUFFERS)
    {
        commandListArrayPointer = &commandPoolItem->CommandLists[commandPoolItem->CurrentCommandListIndex++];
        commandList = *commandListArrayPointer;
        isFromCommandPoolItem = true;
    }

    if (commandList == nullptr)
    {
        if (!isFromCommandPoolItem)
        {
            LogWarningMessage(LogMessageCategory_Graphics, L"Warning: Not enough command buffer objects in the pool. Performance may decrease...");
        } 

        commandList = new VulkanCommandList(commandQueue->GraphicsDevice);
        commandList->CommandQueue = commandQueue;

        VkCommandBufferAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
        allocateInfo.commandPool = commandPoolItem->CommandPool;
        allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocateInfo.commandBufferCount = 1;

        AssertIfFailed(vkAllocateCommandBuffers(graphicsDevice->Device, &allocateInfo, &commandList->DeviceObject));

        commandList->IsFromCommandPool = isFromCommandPoolItem;

        if (isFromCommandPoolItem)
        {
            commandList->CommandPoolItem = commandPoolItem;
            *commandListArrayPointer = commandList;
        }
    }

    assert(commandList != nullptr);
    return commandList;
}
    
Fence VulkanCreateCommandQueueFence(VulkanCommandQueue* commandQueue)
{
    // TODO: Use std::atomic
    auto fenceValue = InterlockedIncrement(&commandQueue->FenceValue);

    VkTimelineSemaphoreSubmitInfo timelineInfo = { VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO };
    timelineInfo.waitSemaphoreValueCount = 0;
    timelineInfo.pWaitSemaphoreValues = nullptr;
    timelineInfo.signalSemaphoreValueCount = 1;
    timelineInfo.pSignalSemaphoreValues = &fenceValue;

    VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submitInfo.pNext = &timelineInfo;
    submitInfo.waitSemaphoreCount = 0;
    submitInfo.pWaitSemaphores = nullptr;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &commandQueue->TimelineSemaphore;
    submitInfo.commandBufferCount = 0;
    submitInfo.pCommandBuffers = nullptr;
    submitInfo.pWaitDstStageMask = 0;

    AssertIfFailed(vkQueueSubmit(commandQueue->DeviceObject, 1, &submitInfo, VK_NULL_HANDLE));

    auto fence = Fence();
    fence.CommandQueuePointer = commandQueue;
    fence.FenceValue = fenceValue;

    return fence;
}

void VulkanCreateSwapChainBackBuffers(VulkanSwapChain* swapChain, int32_t width, int32_t height)
{
    auto graphicsDevice = swapChain->GraphicsDevice;

    uint32_t swapchainImageCount = 0;
    VkImage swapchainImages[3];
    AssertIfFailed(vkGetSwapchainImagesKHR(graphicsDevice->Device, swapChain->DeviceObject, &swapchainImageCount, nullptr));
    AssertIfFailed(vkGetSwapchainImagesKHR(graphicsDevice->Device, swapChain->DeviceObject, &swapchainImageCount, swapchainImages));

    auto format = swapChain->Format == SwapChainFormat_HighDynamicRange ? VK_FORMAT_R16G16B16A16_SFLOAT : VK_FORMAT_B8G8R8A8_SRGB;

    for (uint32_t i = 0; i < swapchainImageCount; i++)
    {
        if (swapChain->BackBufferTextures[i])
        {
            delete swapChain->BackBufferTextures[i];
        }

        auto backBufferTexture = new VulkanTexture(graphicsDevice);
        backBufferTexture->DeviceObject = swapchainImages[i];
        backBufferTexture->IsPresentTexture = true;
        backBufferTexture->Format = format;
        backBufferTexture->Width = (uint32_t)width;
        backBufferTexture->Height = (uint32_t)height;

        VkImageViewCreateInfo createInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        createInfo.image = swapchainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = format;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.layerCount = 1;

        AssertIfFailed(vkCreateImageView(graphicsDevice->Device, &createInfo, 0, &backBufferTexture->ImageView));

        swapChain->BackBufferTextures[i] = backBufferTexture;
    }
}

uint64_t VulkanComputeRenderPipelineStateHash(VulkanShader* shader, RenderPassDescriptor* renderPassDescriptor)
{
    // TODO: For the moment the hash of the shader is base on the pointer
    // Maybe we should base it on the hash of each shader parts data? 
    // This would prevent creating duplicate PSO if 2 shaders contains the same parts (it looks like an edge case)
    // but this would add more processing to generate the hash and this function is perf critical

    // TODO: Hash other render pass parameters
    // TODO: Use FarmHash64? https://github.com/TommasoBelluzzo/FastHashes/tree/master

    return (uint64_t)shader;
}

VulkanPipelineStateCacheItem* VulkanCreateRenderPipelineState(VulkanShader* shader, RenderPassDescriptor* renderPassDescriptor)
{
    auto graphicsDevice = shader->GraphicsDevice;

    VkGraphicsPipelineCreateInfo createInfo = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
    uint32_t stagesCount = 2;

    VkPipelineShaderStageCreateInfo stages[3] = {};
    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_MESH_BIT_EXT;
    stages[0].module = shader->MeshShader;
    stages[0].pName = shader->MeshShaderEntryPoint;

    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = shader->PixelShader;
    stages[1].pName = shader->PixelShaderEntryPoint;
    
    if (shader->AmplificationShader != nullptr)
    {
        stages[2].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[2].stage = VK_SHADER_STAGE_TASK_BIT_EXT;
        stages[2].module = shader->AmplificationShader;
        stages[2].pName = shader->AmplificationShaderEntryPoint;
        stagesCount++;
    }
    
    createInfo.stageCount = stagesCount;
    createInfo.pStages = stages;

    VkPipelineViewportStateCreateInfo viewportState = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;
    createInfo.pViewportState = &viewportState;

    VkPipelineRasterizationStateCreateInfo rasterizationState = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
    rasterizationState.lineWidth = 1.0f;
    rasterizationState.cullMode = VK_CULL_MODE_NONE;//VK_CULL_MODE_BACK_BIT;
    rasterizationState.frontFace = VK_FRONT_FACE_CLOCKWISE;
    createInfo.pRasterizationState = &rasterizationState;

    VkPipelineMultisampleStateCreateInfo multisampleState = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
    multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    createInfo.pMultisampleState = &multisampleState;

    VkPipelineDepthStencilStateCreateInfo depthStencilState = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
    createInfo.pDepthStencilState = &depthStencilState;

    // TODO: To Refactor!    
    VkPipelineColorBlendAttachmentState renderTargetBlendState = {
				false,
				VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO, VK_BLEND_OP_ADD,
				VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO, VK_BLEND_OP_ADD,
				VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
			};

    VkPipelineColorBlendStateCreateInfo colorBlendState = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
    colorBlendState.attachmentCount = 1;
    colorBlendState.pAttachments = &renderTargetBlendState;
    createInfo.pColorBlendState = &colorBlendState;

    VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

    VkPipelineDynamicStateCreateInfo dynamicState = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
    dynamicState.dynamicStateCount = sizeof(dynamicStates) / sizeof(dynamicStates[0]);
    dynamicState.pDynamicStates = dynamicStates;
    createInfo.pDynamicState = &dynamicState;

    VkFormat formats[] = {VK_FORMAT_B8G8R8A8_SRGB};

    VkPipelineRenderingCreateInfo renderingCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
    renderingCreateInfo.colorAttachmentCount = 1; // TODO: Change that
    renderingCreateInfo.pColorAttachmentFormats = formats;
    createInfo.pNext = &renderingCreateInfo;
    createInfo.layout = shader->PipelineLayout;

    VkPipeline pipelineState = nullptr;
    AssertIfFailed(vkCreateGraphicsPipelines(graphicsDevice->Device, nullptr, 1, &createInfo, 0, &pipelineState));

    VulkanPipelineStateCacheItem* cacheItem = new VulkanPipelineStateCacheItem();
    cacheItem->PipelineState = pipelineState;
    cacheItem->Device = graphicsDevice->Device;

    return cacheItem;
}

void VulkanTransitionTextureToState(VulkanCommandList* commandList, VulkanTexture* texture, VkImageLayout sourceState, VkImageLayout destinationState, bool isTransfer)
{
    // TODO: Handle texture accesses, currently we only handle the image layout
    // TODO: Use VkImageMemoryBarrier2. What are the differences?

    VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };

    barrier.srcAccessMask = VK_ACCESS_NONE_KHR;
    barrier.dstAccessMask = VK_ACCESS_NONE_KHR;
    barrier.oldLayout = sourceState;
    barrier.newLayout = destinationState;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = texture->DeviceObject;
    barrier.subresourceRange.aspectMask = texture->Format == VK_FORMAT_D32_SFLOAT ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
    barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

    if (!isTransfer)
    {
        vkCmdPipelineBarrier(commandList->DeviceObject, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, 1, &barrier);
    }

    else
    {
        vkCmdPipelineBarrier(commandList->DeviceObject, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, 1, &barrier);
    }
}

static VkBool32 VKAPI_CALL VulkanDebugReportCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT, uint64_t, size_t, int32_t, const char*, const char* pMessage, void*)
{
    // This silences warnings like "For optimal performance image layout should be VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL instead of GENERAL."
    // We'll assume other performance warnings are also not useful.
    // if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
    // {
    // 	return VK_FALSE;
    // }

    auto messageType = LogMessageType_Debug;

    if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
    {
        messageType = LogMessageType_Error;
    }
    else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
    {
        messageType = LogMessageType_Warning;
    }

    auto convertedString = SystemConvertUtf8ToWideChar(pMessage);
    LogMessage(messageType, LogMessageCategory_Graphics, L"%ls", convertedString);
    SystemFreeConvertedString(convertedString);

    if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
    {
        assert(!"Vulkan validation error encountered!");
    }

    return VK_FALSE;
}

static void VulkanDeletePipelineCacheItem(uint64_t key, void* data)
{
    auto cacheItem = (VulkanPipelineStateCacheItem*)data;
    vkDestroyPipeline(cacheItem->Device, cacheItem->PipelineState, nullptr);
    delete cacheItem;
}