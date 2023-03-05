#ifdef _WINDOWS
#include "WindowsCommon.h"
#endif

#include "VulkanGraphicsService.h"

VulkanGraphicsService::VulkanGraphicsService(GraphicsServiceOptions* options)
{
    _graphicsDiagnostics = options->GraphicsDiagnostics;

    AssertIfFailed(volkInitialize());

    VkApplicationInfo appInfo = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
    appInfo.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo createInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };

    createInfo.pApplicationInfo = &appInfo;

    if (options->GraphicsDiagnostics == GraphicsDiagnostics_Debug)
    {
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

    volkLoadInstance(_vulkanInstance);
    
    if (options->GraphicsDiagnostics == GraphicsDiagnostics_Debug)
    {
        VkDebugReportCallbackCreateInfoEXT createInfo = { VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT };
        createInfo.flags = VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | VK_DEBUG_REPORT_ERROR_BIT_EXT;
        createInfo.pfnCallback = DebugReportCallback;

        AssertIfFailed(vkCreateDebugReportCallbackEXT(_vulkanInstance, &createInfo, 0, &_debugCallback));
    }
}

VulkanGraphicsService::~VulkanGraphicsService()
{
    if (_debugCallback != nullptr)
    {
        vkDestroyDebugReportCallbackEXT(_vulkanInstance, _debugCallback, nullptr);
    }

    if (_vulkanInstance != nullptr)
    {
        vkDestroyInstance(_vulkanInstance, nullptr);
    }
}

void VulkanGraphicsService::GetAvailableGraphicsDevices(GraphicsDeviceInfo* graphicsDevices, int* count)
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

        VkPhysicalDeviceMeshShaderFeaturesEXT meshShaderFeatures = {};
        meshShaderFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT;

        VkPhysicalDeviceFeatures2 features2 = {};
        features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR;
        features2.pNext = &meshShaderFeatures;

        vkGetPhysicalDeviceFeatures2(devices[i], &features2);

        if (meshShaderFeatures.meshShader && meshShaderFeatures.taskShader)
        {
            graphicsDevices[(*count)++] = ConstructGraphicsDeviceInfo(deviceProperties, deviceMemoryProperties);
        }
    }
}

void* VulkanGraphicsService::CreateGraphicsDevice(GraphicsDeviceOptions* options)
{
    printf("Create Vulkan Device\n");

    uint32_t deviceCount = 16;
    VkPhysicalDevice devices[16];
    VkPhysicalDevice physicalDevice = {};
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceMemoryProperties deviceMemoryProperties;
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
    
    auto graphicsDevice = new VulkanGraphicsDevice(this);

    uint32_t queueFamilyCount = 0;
    VkQueueFamilyProperties queueFamilies[32];
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies);

    VkDeviceQueueCreateInfo queueCreateInfos[3];

    for (uint32_t i = 0; i < queueFamilyCount; i++)
    {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            graphicsDevice->RenderCommandQueueFamilyIndex = i;
            queueCreateInfos[i] = CreateDeviceQueueCreateInfo(i, 2);
        }

        else if (queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
        {
            graphicsDevice->ComputeCommandQueueFamilyIndex = i;
            queueCreateInfos[i] = CreateDeviceQueueCreateInfo(i, 1);
        }

        else if (queueFamilies[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
        {
            graphicsDevice->CopyCommandQueueFamilyIndex = i;
            queueCreateInfos[i] = CreateDeviceQueueCreateInfo(i, 1);
        }
    }

    graphicsDevice->InternalId = InterlockedIncrement(&_currentDeviceInternalId) - 1;
    graphicsDevice->PhysicalDevice = physicalDevice;
    graphicsDevice->DeviceProperties = deviceProperties;
    graphicsDevice->DeviceMemoryProperties = deviceMemoryProperties;

    VkDeviceCreateInfo createInfo = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
    createInfo.queueCreateInfoCount = 3;
    createInfo.pQueueCreateInfos = queueCreateInfos;

    const char* extensions[] =
    {
        VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME,
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_SWAPCHAIN_MUTABLE_FORMAT_EXTENSION_NAME,
        VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
        VK_NV_MESH_SHADER_EXTENSION_NAME,
        VK_NV_DEVICE_GENERATED_COMMANDS_EXTENSION_NAME
    };

    createInfo.ppEnabledExtensionNames = extensions;
    createInfo.enabledExtensionCount = ARRAYSIZE(extensions);

    // TODO: Replace that with standard extension but it doesn't seems to work for now :/
    VkPhysicalDeviceMeshShaderFeaturesNV meshFeatures = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_NV };
    meshFeatures.meshShader = true;
    meshFeatures.taskShader = true;

    VkPhysicalDeviceSynchronization2FeaturesKHR sync2Features = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES_KHR };
    sync2Features.synchronization2 = true;
    sync2Features.pNext = &meshFeatures;

    VkPhysicalDeviceVulkan12Features features = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
    features.timelineSemaphore = true;
    features.runtimeDescriptorArray = true;
    features.descriptorIndexing = true;
    features.descriptorBindingVariableDescriptorCount = true;
    features.descriptorBindingPartiallyBound = true;
    features.descriptorBindingSampledImageUpdateAfterBind = true;
    features.descriptorBindingStorageBufferUpdateAfterBind = true;
    features.descriptorBindingStorageImageUpdateAfterBind = true;
    features.shaderSampledImageArrayNonUniformIndexing = true;
    features.separateDepthStencilLayouts = true;
    features.hostQueryReset = true;
    features.shaderInt8 = true;

    if (_graphicsDiagnostics == GraphicsDiagnostics_Debug)
    {
        features.bufferDeviceAddressCaptureReplay = true;
    }

    features.pNext = &sync2Features;
    createInfo.pNext = &features;

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

void VulkanGraphicsService::FreeGraphicsDevice(void* graphicsDevicePointer)
{
    auto graphicsDevice = (VulkanGraphicsDevice*)graphicsDevicePointer;
    VulkanCommandPoolItem* item;

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
    }
}

GraphicsDeviceInfo VulkanGraphicsService::GetGraphicsDeviceInfo(void* graphicsDevicePointer)
{
    auto graphicsDevice = (VulkanGraphicsDevice*)graphicsDevicePointer;
    return ConstructGraphicsDeviceInfo(graphicsDevice->DeviceProperties, graphicsDevice->DeviceMemoryProperties);
}

void* VulkanGraphicsService::CreateCommandQueue(void* graphicsDevicePointer, CommandQueueType type)
{
    auto graphicsDevice = (VulkanGraphicsDevice*)graphicsDevicePointer;

    auto commandQueue = new VulkanCommandQueue(this, graphicsDevice);
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

void VulkanGraphicsService::FreeCommandQueue(void* commandQueuePointer)
{
    auto commandQueue = (VulkanCommandQueue*)commandQueuePointer;
    
    vkDestroySemaphore(commandQueue->GraphicsDevice->Device, commandQueue->TimelineSemaphore, nullptr);
    delete commandQueue;
}

void VulkanGraphicsService::SetCommandQueueLabel(void* commandQueuePointer, uint8_t* label)
{
    auto commandQueue = (VulkanCommandQueue*)commandQueuePointer;
    
    VkDebugUtilsObjectNameInfoEXT nameInfo = { VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT };
    nameInfo.objectType = VK_OBJECT_TYPE_QUEUE;
    nameInfo.objectHandle = (uint64_t)commandQueue->DeviceObject;
    nameInfo.pObjectName = (char*)label;

    AssertIfFailed(vkSetDebugUtilsObjectNameEXT(commandQueue->GraphicsDevice->Device, &nameInfo)); 
}

void* VulkanGraphicsService::CreateCommandList(void* commandQueuePointer)
{
    auto commandQueue = (VulkanCommandQueue*)commandQueuePointer;
    auto graphicsDevice = commandQueue->GraphicsDevice;

    auto commandPool = GetCommandPool(commandQueue);
    auto commandList = GetCommandList(commandQueue, commandPool);

    VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    AssertIfFailed(vkBeginCommandBuffer(commandList->DeviceObject, &beginInfo));

    return commandList;
}

void VulkanGraphicsService::FreeCommandList(void* commandListPointer)
{
    auto commandList = (VulkanCommandList*)commandListPointer;
    
    if (!commandList->IsFromCommandPool)
    {
        delete commandList;
    }
}

void VulkanGraphicsService::SetCommandListLabel(void* commandListPointer, uint8_t* label)
{
    auto commandList = (VulkanCommandList*)commandListPointer;

    VkDebugUtilsObjectNameInfoEXT nameInfo = { VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT };
    nameInfo.objectType = VK_OBJECT_TYPE_COMMAND_BUFFER;
    nameInfo.objectHandle = (uint64_t)commandList->DeviceObject;
    nameInfo.pObjectName = (char*)label;

    AssertIfFailed(vkSetDebugUtilsObjectNameEXT(commandList->GraphicsDevice->Device, &nameInfo));
}

void VulkanGraphicsService::CommitCommandList(void* commandListPointer)
{
    auto commandList = (VulkanCommandList*)commandListPointer;
    AssertIfFailed(vkEndCommandBuffer(commandList->DeviceObject));
}

Fence VulkanGraphicsService::ExecuteCommandLists(void* commandQueuePointer, void** commandLists, int32_t commandListCount, Fence* fencesToWait, int32_t fenceToWaitCount)
{
    auto commandQueue = (VulkanCommandQueue*)commandQueuePointer;

    UpdateCommandPoolFence(commandQueue);
    return Fence();
}

void VulkanGraphicsService::WaitForFenceOnCpu(Fence fence)
{

}

void VulkanGraphicsService::ResetCommandAllocation(void* graphicsDevicePointer)
{
    auto graphicsDevice = (VulkanGraphicsDevice*)graphicsDevicePointer;
    graphicsDevice->CommandPoolGeneration++;
}

void VulkanGraphicsService::FreeTexture(void* texturePointer)
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

void* VulkanGraphicsService::CreateSwapChain(void* windowPointer, void* commandQueuePointer, SwapChainOptions* options)
{
    auto commandQueue = (VulkanCommandQueue*)commandQueuePointer;
    auto graphicsDevice = commandQueue->GraphicsDevice;

    auto swapChain = new VulkanSwapChain(this, graphicsDevice);
    swapChain->CommandQueue = commandQueue;
    swapChain->Format = options->Format;
    swapChain->CurrentImageIndex = 0;

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

    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    AssertIfFailed(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(graphicsDevice->PhysicalDevice, swapChain->WindowSurface, &surfaceCapabilities));

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

    // TODO: Implement Latency await

    AssertIfFailed(vkCreateSwapchainKHR(graphicsDevice->Device, &swapChainCreateInfo, nullptr, &swapChain->DeviceObject));

    CreateSwapChainBackBuffers(swapChain);

    //swapChain->BackBufferAcquireFence = VulkanCreateFence(this->graphicsDevice);
    return swapChain;
}

void VulkanGraphicsService::FreeSwapChain(void* swapChainPointer)
{
	auto swapChain = (VulkanSwapChain*)swapChainPointer;
    auto graphicsDevice = swapChain->GraphicsDevice;

    //vkDestroyFence(this->graphicsDevice, swapChain->BackBufferAcquireFence, nullptr);

    for (int i = 0; i < 3; i++)
    {
        vkDestroyImageView(graphicsDevice->Device, swapChain->BackBufferTextures[i]->ImageView, nullptr);
        delete swapChain->BackBufferTextures[i];
    }

    vkDestroySwapchainKHR(swapChain->GraphicsDevice->Device, swapChain->DeviceObject, nullptr);
    vkDestroySurfaceKHR(_vulkanInstance, swapChain->WindowSurface, nullptr);

    delete swapChain;
}

void VulkanGraphicsService::ResizeSwapChain(void* swapChainPointer, int width, int height)
{

}

void* VulkanGraphicsService::GetSwapChainBackBufferTexture(void* swapChainPointer)
{
    auto swapChain = (VulkanSwapChain*)swapChainPointer;
    return swapChain->BackBufferTextures[swapChain->CurrentImageIndex];
}

void VulkanGraphicsService::PresentSwapChain(void* swapChainPointer)
{
    // TODO: Wait for the correct timeline semaphore value?
    // Or just issue a barrier because the final buffer rendering and the present is done on the same queue
    auto swapChain = (VulkanSwapChain*)swapChainPointer;

    VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
    presentInfo.waitSemaphoreCount = 0;
    // presentInfo.pWaitSemaphores = &releaseSemaphore;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapChain->DeviceObject;
    presentInfo.pImageIndices = &swapChain->CurrentImageIndex;

    //AssertIfFailed(vkQueuePresentKHR(swapChain->CommandQueue->DeviceObject, &presentInfo));
    
    ResetCommandAllocation(swapChain->GraphicsDevice);
}

void VulkanGraphicsService::WaitForSwapChainOnCpu(void* swapChainPointer)
{

}

void VulkanGraphicsService::BeginRenderPass(void* commandListPointer, RenderPassDescriptor* renderPassDescriptor)
{

}
    
void VulkanGraphicsService::EndRenderPass(void* commandListPointer)
{

}
   
GraphicsDeviceInfo VulkanGraphicsService::ConstructGraphicsDeviceInfo(VkPhysicalDeviceProperties deviceProperties, VkPhysicalDeviceMemoryProperties deviceMemoryProperties)
{
    auto result = GraphicsDeviceInfo();
    result.DeviceName = ConvertWStringToUtf8(std::wstring(deviceProperties.deviceName, deviceProperties.deviceName + strlen(deviceProperties.deviceName)));
    result.GraphicsApi = GraphicsApi_Vulkan;
    result.DeviceId = deviceProperties.deviceID;
    result.AvailableMemory = deviceMemoryProperties.memoryHeaps[0].size;

    return result;
}

VkDeviceQueueCreateInfo VulkanGraphicsService::CreateDeviceQueueCreateInfo(uint32_t queueFamilyIndex, uint32_t count)
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
    
VulkanCommandPoolItem* VulkanGraphicsService::GetCommandPool(VulkanCommandQueue* commandQueue)
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
            createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT ;
            createInfo.queueFamilyIndex = commandQueue->FamilyIndex;

            AssertIfFailed(vkCreateCommandPool(graphicsDevice->Device, &createInfo, 0, &commandPoolItem->CommandPool));
        }
        else
        {
            if (commandPoolItem->Fence.FenceValue > 0)
            {
                WaitForFenceOnCpu(commandPoolItem->Fence);
            }

            AssertIfFailed(vkResetCommandPool(graphicsDevice->Device, commandPoolItem->CommandPool, 0));
            commandPoolItem->CurrentCommandListIndex = 0;
        }

        *commandPoolItemPointer = commandPoolItem;
    }

    return (*commandPoolItemPointer);
}
    
void VulkanGraphicsService::UpdateCommandPoolFence(VulkanCommandQueue* commandQueue)
{
    auto graphicsDevice = commandQueue->GraphicsDevice;

    auto fence = Fence();
    fence.CommandQueuePointer = commandQueue;
    fence.FenceValue = commandQueue->FenceValue;

    VulkanDeviceCommandPools& commandPoolsCache = CommandPools[graphicsDevice->InternalId];

    if (commandQueue->CommandQueueType == CommandQueueType_Copy)
    {
        assert(commandPoolsCache.CopyCommandPool != nullptr);
        commandPoolsCache.CopyCommandPool->Fence = fence;
    }
    else if (commandQueue->CommandQueueType == CommandQueueType_Compute)
    {
        assert(commandPoolsCache.ComputeCommandPool != nullptr);
        commandPoolsCache.ComputeCommandPool->Fence = fence;
    }
    else
    {
        assert(commandPoolsCache.DirectCommandPool != nullptr);
        commandPoolsCache.DirectCommandPool->Fence = fence;
    }
}

uint32_t _clCounter = 0;
VulkanCommandList* VulkanGraphicsService::GetCommandList(VulkanCommandQueue* commandQueue, VulkanCommandPoolItem* commandPoolItem)
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
        // BUG: Is there a bug? If we increase the thread count in the test program
        // We should get the same amount of command list creation. Maybe it is the case for the moment because
        // the fence is always 0.

        printf("Create CommandBuffer %d...\n", _clCounter++);
        
        if (!isFromCommandPoolItem)
        {
            printf("Warning: Not enough command buffer objects in the pool. Performance may decrease...\n");
        } 

        commandList = new VulkanCommandList(this, commandQueue->GraphicsDevice);
        commandList->CommandQueue = commandQueue;
        
        VkCommandBufferAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
        allocateInfo.commandPool = commandPoolItem->CommandPool;
        allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocateInfo.commandBufferCount = 1;

        AssertIfFailed(vkAllocateCommandBuffers(graphicsDevice->Device, &allocateInfo, &commandList->DeviceObject));

        commandList->IsUsed = true;
        commandList->IsFromCommandPool = true;

        if (isFromCommandPoolItem)
        {
            *commandListArrayPointer = commandList;
        }
    }

    assert(commandList != nullptr);
    return commandList;
}

void VulkanGraphicsService::CreateSwapChainBackBuffers(VulkanSwapChain* swapChain)
{
    auto graphicsDevice = swapChain->GraphicsDevice;

    uint32_t swapchainImageCount = 0;
    VkImage swapchainImages[3];
	AssertIfFailed(vkGetSwapchainImagesKHR(graphicsDevice->Device, swapChain->DeviceObject, &swapchainImageCount, nullptr));
	AssertIfFailed(vkGetSwapchainImagesKHR(graphicsDevice->Device, swapChain->DeviceObject, &swapchainImageCount, swapchainImages));

    auto format = swapChain->Format == SwapChainFormat_HighDynamicRange ? VK_FORMAT_R16G16B16A16_SFLOAT : VK_FORMAT_B8G8R8A8_SRGB;

    for (uint32_t i = 0; i < swapchainImageCount; i++)
    {
        auto backBufferTexture = new VulkanTexture(this, graphicsDevice);
        backBufferTexture->DeviceObject = swapchainImages[i];
        backBufferTexture->IsPresentTexture = true;

        VkImageViewCreateInfo createInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        createInfo.image = swapchainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = format;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.layerCount = 1;

        VkImageView view = nullptr;
        AssertIfFailed(vkCreateImageView(graphicsDevice->Device, &createInfo, 0, &backBufferTexture->ImageView));    

        swapChain->BackBufferTextures[i] = backBufferTexture;
    }
}

static VkBool32 VKAPI_CALL DebugReportCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage, void* pUserData)
{
	// This silences warnings like "For optimal performance image layout should be VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL instead of GENERAL."
	// We'll assume other performance warnings are also not useful.
	// if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
    // {
	// 	return VK_FALSE;
    // }

	const char* type = "[93mVULKAN INFO";

	if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
    {
        type = "[91mVULKAN ERROR";
    }

    else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
    {
	    type = "[93mVULKAN WARNING";
    }

	char message[4096];
	snprintf(message, 4096, "%s: %s[0m\n", type, pMessage);

	printf("%s", message);

#ifdef _WIN32
	OutputDebugStringA(message);
#endif

	if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
    {
		assert(!"Vulkan validation error encountered!");
    }

	return VK_FALSE;
}