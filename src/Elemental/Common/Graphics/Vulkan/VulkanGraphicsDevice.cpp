#include "VulkanGraphicsDevice.h"
#include "SystemDataPool.h"
#include "SystemFunctions.h"
#include "SystemLogging.h"
#include "SystemMemory.h"

#define VULKAN_MAX_DEVICES 10u

MemoryArena VulkanGraphicsMemoryArena;
SystemDataPool<VulkanGraphicsDeviceData, VulkanGraphicsDeviceDataFull> vulkanGraphicsDevicePool;

bool vulkanDebugLayerEnabled = false;
VkInstance vulkanInstance = nullptr;
VkDebugReportCallbackEXT vulkanDebugCallback = nullptr;

VkBool32 VKAPI_CALL VulkanDebugReportCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT, uint64_t, size_t, int32_t, const char*, const char* pMessage, void*)
{
    auto messageType = ElemLogMessageType_Debug;

    if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
    {
        messageType = ElemLogMessageType_Error;
    }
    else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
    {
        messageType = ElemLogMessageType_Warning;
    }

    SystemLogMessage(messageType, ElemLogMessageCategory_Graphics, "%s", pMessage);

    if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
    {
        SystemAssert(!"Vulkan validation error encountered!");
    }

    return VK_FALSE;
}

void InitVulkan()
{
    auto stackMemoryArena = SystemGetStackMemoryArena();

    AssertIfFailed(volkInitialize());
    SystemAssert(volkGetInstanceVersion() >= VK_API_VERSION_1_3);

    VkApplicationInfo appInfo = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
    appInfo.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo createInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
    createInfo.pApplicationInfo = &appInfo;
    
    // TODO: If the vulkan loader is not here, don't proceed
    auto isSdkInstalled = SystemLoadLibrary("VkLayer_khronos_validation").Handle != nullptr;

    if (vulkanDebugLayerEnabled)
    {
        if (isSdkInstalled)
        {
            SystemLogDebugMessage(ElemLogMessageCategory_Graphics, "Init Vulkan Debug Mode");

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
                #ifdef _WIN32
                VK_KHR_WIN32_SURFACE_EXTENSION_NAME
                #endif
                #ifdef __APPLE__
                VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME
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

            #ifdef __APPLE__
            createInfo.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
            #endif

            createInfo.pNext = &validationFeatures;

            AssertIfFailed(vkCreateInstance(&createInfo, nullptr, &vulkanInstance));

        }
        else
        {
            SystemLogWarningMessage(ElemLogMessageCategory_Graphics, "VkLayer_khronos_validation not found but EnableGraphicsDebugLayer() was called. Debug layer will not be enabled."); 
        }
    }
    else
    {
        SystemLogDebugMessage(ElemLogMessageCategory_Graphics, "Init Vulkan..."); 

        const char* extensions[] =
        {
            VK_KHR_SURFACE_EXTENSION_NAME,
            #ifdef WIN32
            VK_KHR_WIN32_SURFACE_EXTENSION_NAME
            #endif
            #ifdef __APPLE__
            VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME
            #endif
        };
        
        createInfo.ppEnabledExtensionNames = extensions;
        createInfo.enabledExtensionCount = ARRAYSIZE(extensions);

        #ifdef __APPLE__
        createInfo.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
        #endif

        AssertIfFailed(vkCreateInstance(&createInfo, nullptr, &vulkanInstance));
    }

    volkLoadInstanceOnly(vulkanInstance);

    if (vulkanDebugLayerEnabled && isSdkInstalled)
    {
        VkDebugReportCallbackCreateInfoEXT debugCreateInfo = { VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT };
        debugCreateInfo.flags = VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | VK_DEBUG_REPORT_ERROR_BIT_EXT;
        debugCreateInfo.pfnCallback = VulkanDebugReportCallback;

        AssertIfFailed(vkCreateDebugReportCallbackEXT(vulkanInstance, &debugCreateInfo, 0, &vulkanDebugCallback));
    }
}

void InitVulkanGraphicsDeviceMemory()
{
    if (!VulkanGraphicsMemoryArena.Storage)
    {
        VulkanGraphicsMemoryArena = SystemAllocateMemoryArena();
        vulkanGraphicsDevicePool = SystemCreateDataPool<VulkanGraphicsDeviceData, VulkanGraphicsDeviceDataFull>(VulkanGraphicsMemoryArena, VULKAN_MAX_DEVICES);

        InitVulkan();
    }
}

ElemGraphicsDeviceInfo VulkanConstructGraphicsDeviceInfo(VkPhysicalDeviceProperties deviceProperties, VkPhysicalDeviceMemoryProperties deviceMemoryProperties)
{
    auto stackMemoryArena = SystemGetStackMemoryArena();

    auto deviceName = ReadOnlySpan<char>(deviceProperties.deviceName);
    auto destinationDeviceName = SystemPushArray<char>(stackMemoryArena, deviceName.Length);
    SystemCopyBuffer<char>(destinationDeviceName, deviceName);

    return 
    {
        .DeviceName = destinationDeviceName.Pointer,
        .GraphicsApi = ElemGraphicsApi_Vulkan,
        .DeviceId = deviceProperties.deviceID,
        .AvailableMemory = deviceMemoryProperties.memoryHeaps[0].size
    };
}

bool VulkanCheckGraphicsDeviceCompatibility(VkPhysicalDevice device)
{
    #ifdef __APPLE__
    return true;
    #else
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    VkPhysicalDevicePresentIdFeaturesKHR presentIdFeatures {};
    presentIdFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_ID_FEATURES_KHR;

    VkPhysicalDeviceMeshShaderFeaturesEXT meshShaderFeatures = {};
    meshShaderFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT;
    meshShaderFeatures.pNext = &presentIdFeatures;

    VkPhysicalDeviceFeatures2 features2 = {};
    features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR;

    #ifndef __APPLE__
    features2.pNext = &meshShaderFeatures;
    #endif

    vkGetPhysicalDeviceFeatures2(device, &features2);
    
    if (meshShaderFeatures.meshShader && meshShaderFeatures.taskShader && presentIdFeatures.presentId)
    {
        return true;
    }
    
    return false;
    #endif
}

VulkanGraphicsDeviceData* GetVulkanGraphicsDeviceData(ElemGraphicsDevice graphicsDevice)
{
    return SystemGetDataPoolItem(vulkanGraphicsDevicePool, graphicsDevice);
}

VulkanGraphicsDeviceDataFull* GetVulkanGraphicsDeviceDataFull(ElemGraphicsDevice graphicsDevice)
{
    return SystemGetDataPoolItemFull(vulkanGraphicsDevicePool, graphicsDevice);
}

void VulkanEnableGraphicsDebugLayer()
{
    vulkanDebugLayerEnabled = true;
}

ElemGraphicsDeviceInfoSpan VulkanGetAvailableGraphicsDevices()
{
    InitVulkanGraphicsDeviceMemory();

    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto deviceInfos = SystemPushArray<ElemGraphicsDeviceInfo>(stackMemoryArena, VULKAN_MAX_DEVICES);
    auto currentDeviceInfoIndex = 0u;

    uint32_t deviceCount = VULKAN_MAX_DEVICES;
    VkPhysicalDevice devices[VULKAN_MAX_DEVICES];

    AssertIfFailed(vkEnumeratePhysicalDevices(vulkanInstance, &deviceCount, nullptr));
    AssertIfFailed(vkEnumeratePhysicalDevices(vulkanInstance, &deviceCount, devices));

    for (uint32_t i = 0; i < deviceCount; i++)
    {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(devices[i], &deviceProperties);

        VkPhysicalDeviceMemoryProperties deviceMemoryProperties;
        vkGetPhysicalDeviceMemoryProperties(devices[i], &deviceMemoryProperties);

        if (VulkanCheckGraphicsDeviceCompatibility(devices[i]))
        {
            deviceInfos[currentDeviceInfoIndex++] = VulkanConstructGraphicsDeviceInfo(deviceProperties, deviceMemoryProperties);
        }
    }

    return
    {
        .Items = deviceInfos.Pointer,
        .Length = currentDeviceInfoIndex
    };
}

ElemGraphicsDevice VulkanCreateGraphicsDevice(const ElemGraphicsDeviceOptions* options)
{
    // TODO: Review features selection
    InitVulkanGraphicsDeviceMemory();

    auto deviceCount = VULKAN_MAX_DEVICES;
    VkPhysicalDevice devices[VULKAN_MAX_DEVICES];
    VkPhysicalDevice physicalDevice = {};
    VkPhysicalDeviceProperties deviceProperties = {};
    VkPhysicalDeviceMemoryProperties deviceMemoryProperties {};
    auto foundDevice = false;

    AssertIfFailed(vkEnumeratePhysicalDevices(vulkanInstance, &deviceCount, nullptr));
    AssertIfFailed(vkEnumeratePhysicalDevices(vulkanInstance, &deviceCount, devices));

    for (uint32_t i = 0; i < deviceCount; i++)
    {
        if (VulkanCheckGraphicsDeviceCompatibility(devices[i]))
        {
            vkGetPhysicalDeviceProperties(devices[i], &deviceProperties);
            vkGetPhysicalDeviceMemoryProperties(devices[i], &deviceMemoryProperties);

            if ((options != nullptr && options->DeviceId == deviceProperties.deviceID) || options == nullptr || options->DeviceId == 0)
            {
                physicalDevice = devices[i];
                foundDevice = true;
                break;
            }
        }
    }

    SystemAssertReturnNullHandle(foundDevice);

    uint32_t queueFamilyCount = 0;
    VkQueueFamilyProperties queueFamilies[32];
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies);

    // TODO: Review command queue selection (before we were creating 2 render command queues, is it needed?)
    // TODO: We don't know how many queues the consumers will create so we need to foreseen some room sticking for 1 for now
    VkDeviceQueueCreateInfo queueCreateInfos[3];
    uint32_t renderCommandQueue = UINT32_MAX;
    uint32_t computeCommandQueue = UINT32_MAX;
    uint32_t copyCommandQueue = UINT32_MAX;
    float queuePriority = 1.0f;

    for (uint32_t i = 0; i < 3; i++)
    {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT && renderCommandQueue == UINT32_MAX)
        {
            renderCommandQueue = i;
        }

        else if (queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT && computeCommandQueue == UINT32_MAX)
        {
            computeCommandQueue = i;
        }

        else if (queueFamilies[i].queueFlags & VK_QUEUE_TRANSFER_BIT && copyCommandQueue == UINT32_MAX)
        {
            copyCommandQueue = i;
        }

        VkDeviceQueueCreateInfo queueCreateInfo = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.queueFamilyIndex = i;

        queueCreateInfos[i] = queueCreateInfo;
    }

    VkDeviceCreateInfo createInfo = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
    createInfo.queueCreateInfoCount = 3; // TODO: To Review
    createInfo.pQueueCreateInfos = queueCreateInfos;

    const char* extensions[] =
    {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        #ifndef __APPLE__ // TODO: Not supported yet on MacOS
        VK_EXT_MESH_SHADER_EXTENSION_NAME
        #endif
        #ifdef __APPLE__
        VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME
        #endif
    };

    createInfo.ppEnabledExtensionNames = extensions;
    createInfo.enabledExtensionCount = ARRAYSIZE(extensions);

    VkPhysicalDeviceFeatures2 features = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
    features.features.multiDrawIndirect = true;
    features.features.shaderInt16 = true;
    features.features.shaderInt64 = true;

    #ifndef __APPLE__
    features.features.pipelineStatisticsQuery = true;
    #endif

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

    if (vulkanDebugLayerEnabled)
    {
        #ifndef __APPLE__
        features12.bufferDeviceAddressCaptureReplay = true;
        #endif
    }

    VkPhysicalDeviceVulkan13Features features13 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
    features13.maintenance4 = true;
    features13.synchronization2 = true;
    features13.dynamicRendering = true;

    VkPhysicalDeviceMeshShaderFeaturesEXT meshFeatures = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT };
    meshFeatures.meshShader = true;
    meshFeatures.taskShader = true;
    meshFeatures.meshShaderQueries = true;

    createInfo.pNext = &features;
    features.pNext = &features12;
    features12.pNext = &features13;
    features13.pNext = &meshFeatures;

    VkDevice device = nullptr;
    AssertIfFailedReturnNullHandle(vkCreateDevice(physicalDevice, &createInfo, nullptr, &device));
    volkLoadDevice(device);

    auto handle = SystemAddDataPoolItem(vulkanGraphicsDevicePool, {
        .Device = device
    }); 

    SystemAddDataPoolItemFull(vulkanGraphicsDevicePool, handle, {
        .PhysicalDevice = physicalDevice,
        .DeviceProperties = deviceProperties,
        .DeviceMemoryProperties = deviceMemoryProperties
    });

    return handle;
}

void VulkanFreeGraphicsDevice(ElemGraphicsDevice graphicsDevice)
{
    SystemAssert(graphicsDevice != ELEM_HANDLE_NULL);

    auto graphicsDeviceData = GetVulkanGraphicsDeviceData(graphicsDevice);
    SystemAssert(graphicsDeviceData);

    auto graphicsDeviceDataFull = GetVulkanGraphicsDeviceDataFull(graphicsDevice);
    SystemAssert(graphicsDeviceDataFull);
}

ElemGraphicsDeviceInfo VulkanGetGraphicsDeviceInfo(ElemGraphicsDevice graphicsDevice)
{
    SystemAssert(graphicsDevice != ELEM_HANDLE_NULL);

    auto graphicsDeviceDataFull = GetVulkanGraphicsDeviceDataFull(graphicsDevice);
    SystemAssert(graphicsDeviceDataFull);

    return VulkanConstructGraphicsDeviceInfo(graphicsDeviceDataFull->DeviceProperties, graphicsDeviceDataFull->DeviceMemoryProperties);
}
