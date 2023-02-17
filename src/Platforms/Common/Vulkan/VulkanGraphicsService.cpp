#ifdef _WINDOWS
#include "WindowsCommon.h"
#endif

#include "VulkanGraphicsService.h"

VulkanGraphicsService::VulkanGraphicsService(GraphicsServiceOptions options)
{
    _graphicsDiagnostics = options.GraphicsDiagnostics;

    AssertIfFailed(volkInitialize());

    VkApplicationInfo appInfo = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
    appInfo.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo createInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };

    createInfo.pApplicationInfo = &appInfo;

    if (options.GraphicsDiagnostics == GraphicsDiagnostics_Debug)
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
    
    if (options.GraphicsDiagnostics == GraphicsDiagnostics_Debug)
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

void* VulkanGraphicsService::CreateGraphicsDevice(GraphicsDeviceOptions options)
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

        if (deviceProperties.deviceID == options.DeviceId)
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
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

    VkQueueFamilyProperties queueFamilies[32];
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
    return nullptr;
}

void VulkanGraphicsService::FreeCommandQueue(void* commandQueuePointer)
{

}

void VulkanGraphicsService::SetCommandQueueLabel(void* commandQueuePointer, uint8_t* label)
{

}

void* VulkanGraphicsService::CreateCommandList(void* commandQueuePointer)
{
    return nullptr;
}

void VulkanGraphicsService::FreeCommandList(void* commandListPointer)
{

}

void VulkanGraphicsService::SetCommandListLabel(void* commandListPointer, uint8_t* label)
{

}

void VulkanGraphicsService::CommitCommandList(void* commandListPointer)
{

}

Fence VulkanGraphicsService::ExecuteCommandLists(void* commandQueuePointer, void** commandLists, int32_t commandListCount, Fence* fencesToWait, int32_t fenceToWaitCount)
{
    return Fence();
}

void VulkanGraphicsService::WaitForFenceOnCpu(Fence fence)
{

}

void* VulkanGraphicsService::CreateSwapChain(void* windowPointer, void* commandQueuePointer, SwapChainOptions options)
{
    return nullptr;
}

void VulkanGraphicsService::FreeSwapChain(void* swapChainPointer)
{

}

void* VulkanGraphicsService::GetSwapChainBackBufferTexture(void* swapChainPointer)
{
    return nullptr;
}

void VulkanGraphicsService::PresentSwapChain(void* swapChainPointer)
{

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