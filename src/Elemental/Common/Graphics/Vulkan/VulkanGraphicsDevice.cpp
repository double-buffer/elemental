#include "VulkanGraphicsDevice.h"
#include "SystemDataPool.h"
#include "SystemFunctions.h"
#include "SystemLogging.h"
#include "SystemMemory.h"

MemoryArena VulkanGraphicsMemoryArena;
SystemDataPool<VulkanGraphicsDeviceData, VulkanGraphicsDeviceDataFull> vulkanGraphicsDevicePool;

bool VulkanDebugLayerEnabled = false;
bool vulkanDebugGpuValidationEnabled = false;
bool VulkanDebugBarrierInfoEnabled = false;
VkInstance VulkanInstance = nullptr;
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

    if (SystemFindSubString(pMessage, "VK_EXT_mutable_descriptor_type") != -1)
    {
        return VK_FALSE;
    }

    SystemLogMessage(messageType, ElemLogMessageCategory_Graphics, "%s", pMessage);

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
    
    auto isSdkInstalled = false;
    auto instanceCreated = false;

    uint32_t instanceLayerCount;
    vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr);

    auto instanceLayers = SystemPushArray<VkLayerProperties>(stackMemoryArena, instanceLayerCount);
    vkEnumerateInstanceLayerProperties(&instanceLayerCount, instanceLayers.Pointer);

    for (uint32_t i = 0; i < instanceLayerCount; i++)
    {
        // TODO: Use a system function for that
        if (strcmp(instanceLayers[i].layerName, "VK_LAYER_KHRONOS_validation") == 0)
        {
            isSdkInstalled = true;
            break;
        }
    }

    // TODO: Enumerate instance extensions like in the cube sample to see if the system
    // is compatible with the current surface extension

    if (VulkanDebugLayerEnabled)
    {
        if (isSdkInstalled)
        {
            SystemLogDebugMessage(ElemLogMessageCategory_Graphics, "Init Vulkan Debug Mode.");

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
                #elif __linux__
                VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME
                #endif
            };

            createInfo.ppEnabledExtensionNames = extensions;
            createInfo.enabledExtensionCount = ARRAYSIZE(extensions);

            auto enabledValidationFeatures = SystemPushArray<VkValidationFeatureEnableEXT>(stackMemoryArena, 3);
            auto currentEnabledValidationFeaturesIndex = 0u;

            enabledValidationFeatures[currentEnabledValidationFeaturesIndex++] = VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT;
            enabledValidationFeatures[currentEnabledValidationFeaturesIndex++] = VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT;

            if (vulkanDebugGpuValidationEnabled)
            {
                enabledValidationFeatures[currentEnabledValidationFeaturesIndex++] = VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT;
            }

            VkValidationFeaturesEXT validationFeatures = { VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT };
            validationFeatures.enabledValidationFeatureCount = currentEnabledValidationFeaturesIndex;
            validationFeatures.pEnabledValidationFeatures = enabledValidationFeatures.Pointer;

            createInfo.pNext = &validationFeatures;

            AssertIfFailed(vkCreateInstance(&createInfo, nullptr, &VulkanInstance));
            instanceCreated = true;

        }
        else
        {
            SystemLogWarningMessage(ElemLogMessageCategory_Graphics, "VkLayer_khronos_validation not found but EnableGraphicsDebugLayer() was called. Debug layer will not be enabled."); 
            VulkanDebugLayerEnabled = false;
        }
    }
    
    if (!instanceCreated)
    {
        SystemLogDebugMessage(ElemLogMessageCategory_Graphics, "Init Vulkan..."); 

        const char* extensions[] =
        {
            VK_KHR_SURFACE_EXTENSION_NAME,
            #ifdef WIN32
            VK_KHR_WIN32_SURFACE_EXTENSION_NAME
            #elif __linux__
            VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME
            #endif
        };
        
        createInfo.ppEnabledExtensionNames = extensions;
        createInfo.enabledExtensionCount = ARRAYSIZE(extensions);

        AssertIfFailed(vkCreateInstance(&createInfo, nullptr, &VulkanInstance));
    }

    volkLoadInstanceOnly(VulkanInstance);

    if (VulkanDebugLayerEnabled && isSdkInstalled)
    {
        VkDebugReportCallbackCreateInfoEXT debugCreateInfo = { VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT };
        debugCreateInfo.flags = VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_ERROR_BIT_EXT;
        debugCreateInfo.pfnCallback = VulkanDebugReportCallback;

        AssertIfFailed(vkCreateDebugReportCallbackEXT(VulkanInstance, &debugCreateInfo, 0, &vulkanDebugCallback));
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

ElemGraphicsDeviceInfo VulkanConstructGraphicsDeviceInfo(MemoryArena memoryArena, VkPhysicalDeviceProperties deviceProperties, VkPhysicalDeviceMemoryProperties deviceMemoryProperties)
{
    auto deviceName = ReadOnlySpan<char>(deviceProperties.deviceName);
    auto destinationDeviceName = SystemPushArray<char>(memoryArena, deviceName.Length);
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
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    VkPhysicalDevicePresentIdFeaturesKHR presentIdFeatures { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_ID_FEATURES_KHR };

    VkPhysicalDeviceMeshShaderFeaturesEXT meshShaderFeatures = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT };
    meshShaderFeatures.pNext = &presentIdFeatures;

    VkPhysicalDeviceFeatures2 features2 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR };
    features2.pNext = &meshShaderFeatures;

    vkGetPhysicalDeviceFeatures2(device, &features2);
    
    if (meshShaderFeatures.meshShader && meshShaderFeatures.taskShader && presentIdFeatures.presentId)
    {
        return true;
    }
    
    return false;
}

VulkanGraphicsDeviceData* GetVulkanGraphicsDeviceData(ElemGraphicsDevice graphicsDevice)
{
    return SystemGetDataPoolItem(vulkanGraphicsDevicePool, graphicsDevice);
}

VulkanGraphicsDeviceDataFull* GetVulkanGraphicsDeviceDataFull(ElemGraphicsDevice graphicsDevice)
{
    return SystemGetDataPoolItemFull(vulkanGraphicsDevicePool, graphicsDevice);
}

void CreateVulkanPipelineLayout(ElemGraphicsDevice graphicsDevice)
{
    auto graphicsDeviceData = GetVulkanGraphicsDeviceData(graphicsDevice);
    SystemAssert(graphicsDeviceData);

    auto graphicsDeviceDataFull = GetVulkanGraphicsDeviceDataFull(graphicsDevice);
    SystemAssert(graphicsDeviceDataFull);

    VkPipelineLayoutCreateInfo layoutCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	layoutCreateInfo.pSetLayouts = nullptr;
	layoutCreateInfo.setLayoutCount = 0;

    VkPushConstantRange push_constant;
    push_constant.offset = 0;
    push_constant.size = 24 * 4;
    push_constant.stageFlags = VK_SHADER_STAGE_ALL;

    layoutCreateInfo.pPushConstantRanges = &push_constant;
    layoutCreateInfo.pushConstantRangeCount = 1;

    VkDescriptorType resourceDescriptorTypes[] = { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE }; 

    VkMutableDescriptorTypeListEXT descriptorSetTypeList = 
    { 
        .pDescriptorTypes = resourceDescriptorTypes,
        .descriptorTypeCount = ARRAYSIZE(resourceDescriptorTypes)
    };

    VkMutableDescriptorTypeCreateInfoEXT mutableDescriptorTypeCreateInfo = { VK_STRUCTURE_TYPE_MUTABLE_DESCRIPTOR_TYPE_CREATE_INFO_EXT };
    mutableDescriptorTypeCreateInfo.pMutableDescriptorTypeLists = &descriptorSetTypeList;
    mutableDescriptorTypeCreateInfo.mutableDescriptorTypeListCount = 1;

    VkDescriptorBindingFlags descriptorBindingFlags = VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT | 
                                                      VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | 
                                                      VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;

	VkDescriptorSetLayoutBindingFlagsCreateInfo descriptorBindingFlagsCreateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO };
	descriptorBindingFlagsCreateInfo.bindingCount = 1;
	descriptorBindingFlagsCreateInfo.pBindingFlags = &descriptorBindingFlags;
    descriptorBindingFlagsCreateInfo.pNext = &mutableDescriptorTypeCreateInfo;

	VkDescriptorSetLayoutBinding descriptorBinding = {};
	descriptorBinding.binding = 0;
	descriptorBinding.descriptorType = VK_DESCRIPTOR_TYPE_MUTABLE_EXT;
	descriptorBinding.descriptorCount = VULKAN_MAX_RESOURCES;
	descriptorBinding.stageFlags = VK_SHADER_STAGE_ALL;

	VkDescriptorSetLayoutCreateInfo descriptorSetCreateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	descriptorSetCreateInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
	descriptorSetCreateInfo.bindingCount = 1;
	descriptorSetCreateInfo.pBindings = &descriptorBinding;
	descriptorSetCreateInfo.pNext = &descriptorBindingFlagsCreateInfo;

	AssertIfFailed(vkCreateDescriptorSetLayout(graphicsDeviceData->Device, &descriptorSetCreateInfo, 0, &graphicsDeviceDataFull->ResourceDescriptorSetLayout));

    layoutCreateInfo.pSetLayouts = &graphicsDeviceDataFull->ResourceDescriptorSetLayout;
    layoutCreateInfo.setLayoutCount = 1;

	AssertIfFailed(vkCreatePipelineLayout(graphicsDeviceData->Device, &layoutCreateInfo, 0, &graphicsDeviceData->PipelineLayout));
}

void VulkanSetGraphicsOptions(const ElemGraphicsOptions* options)
{
    SystemAssert(options);

    if (options->EnableDebugLayer)
    {
        VulkanDebugLayerEnabled = options->EnableDebugLayer;
    }

    if (options->EnableGpuValidation)
    {
        vulkanDebugGpuValidationEnabled = options->EnableGpuValidation;
    }

    if (options->EnableDebugBarrierInfo)
    {
        VulkanDebugBarrierInfoEnabled = options->EnableDebugBarrierInfo;
    }
}

ElemGraphicsDeviceInfoSpan VulkanGetAvailableGraphicsDevices()
{
    InitVulkanGraphicsDeviceMemory();

    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto deviceInfos = SystemPushArray<ElemGraphicsDeviceInfo>(stackMemoryArena, VULKAN_MAX_DEVICES);
    auto currentDeviceInfoIndex = 0u;

    uint32_t deviceCount = VULKAN_MAX_DEVICES;
    AssertIfFailed(vkEnumeratePhysicalDevices(VulkanInstance, &deviceCount, nullptr));

    auto devices = SystemPushArray<VkPhysicalDevice>(stackMemoryArena, deviceCount);
    AssertIfFailed(vkEnumeratePhysicalDevices(VulkanInstance, &deviceCount, devices.Pointer));

    for (uint32_t i = 0; i < deviceCount; i++)
    {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(devices[i], &deviceProperties);

        VkPhysicalDeviceMemoryProperties deviceMemoryProperties;
        vkGetPhysicalDeviceMemoryProperties(devices[i], &deviceMemoryProperties);

        if (VulkanCheckGraphicsDeviceCompatibility(devices[i]))
        {
            deviceInfos[currentDeviceInfoIndex++] = VulkanConstructGraphicsDeviceInfo(stackMemoryArena, deviceProperties, deviceMemoryProperties);
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
    
    auto stackMemoryArena = SystemGetStackMemoryArena();

    VkPhysicalDevice physicalDevice = {};
    VkPhysicalDeviceProperties deviceProperties = {};
    VkPhysicalDeviceMemoryProperties deviceMemoryProperties {};
    auto foundDevice = false;

    uint32_t deviceCount;
    AssertIfFailed(vkEnumeratePhysicalDevices(VulkanInstance, &deviceCount, nullptr));

    auto devices = SystemPushArray<VkPhysicalDevice>(stackMemoryArena, deviceCount);
    AssertIfFailed(vkEnumeratePhysicalDevices(VulkanInstance, &deviceCount, devices.Pointer));

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
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

    auto queueFamilies = SystemPushArray<VkQueueFamilyProperties>(stackMemoryArena, queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.Pointer);

    VkDeviceQueueCreateInfo queueCreateInfos[3];
    uint32_t renderCommandQueueIndex = UINT32_MAX;
    uint32_t computeCommandQueueIndex = UINT32_MAX;
    uint32_t copyCommandQueueIndex = UINT32_MAX;
    float queuePriority[3] = { 1.0f, 1.0f, 1.0f };

    for (uint32_t i = 0; i < 3; i++)
    {
        uint32_t queueCount = 1;

        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT && renderCommandQueueIndex == UINT32_MAX)
        {
            renderCommandQueueIndex = i;
            queueCount = SystemMin(queueFamilies[i].queueCount, 3u);
        }
        else if (queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT && computeCommandQueueIndex == UINT32_MAX)
        {
            computeCommandQueueIndex = i;
            queueCount = SystemMin(queueFamilies[i].queueCount, 2u);
        }
        else if (queueFamilies[i].queueFlags & VK_QUEUE_TRANSFER_BIT && copyCommandQueueIndex == UINT32_MAX)
        {
            copyCommandQueueIndex = i;
            queueCount = SystemMin(queueFamilies[i].queueCount, 2u);
        }
        else
        {
            SystemLogErrorMessage(ElemLogMessageCategory_Graphics, "Wrong queue type.");
        }
            
        VkDeviceQueueCreateInfo queueCreateInfo = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
        queueCreateInfo.pQueuePriorities = queuePriority;
        queueCreateInfo.queueCount = queueCount;
        queueCreateInfo.queueFamilyIndex = i;

        queueCreateInfos[i] = queueCreateInfo;
    }

    int32_t gpuMemoryTypeIndex = -1;
    int32_t gpuUploadMemoryTypeIndex = -1;
    int32_t readBackMemoryTypeIndex = -1;

    for (uint32_t i = 0; i < deviceMemoryProperties.memoryTypeCount; i++)
    {
        auto memoryPropertyFlags = deviceMemoryProperties.memoryTypes[i].propertyFlags;

        if ((memoryPropertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) && 
            (memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == 0)
        {
            gpuMemoryTypeIndex = i;
        }
        if ((memoryPropertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) && 
            (memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT))
        {
            gpuUploadMemoryTypeIndex = i;
        }
        else if ((memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) && 
                 (memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) && 
                 (memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT))
        {
            readBackMemoryTypeIndex = i;
        }

        // TODO: Later we will need that to implement IOGraphicsQueue
        /*
        else if ((memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) && (memoryPropertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) == 0)
        {
        }*/
    }

    SystemAssert(gpuMemoryTypeIndex != -1 && gpuUploadMemoryTypeIndex != -1 && readBackMemoryTypeIndex != -1);

    VkDeviceCreateInfo createInfo = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
    createInfo.queueCreateInfoCount = 3;
    createInfo.pQueueCreateInfos = queueCreateInfos;

    const char* extensions[] =
    {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_PRESENT_ID_EXTENSION_NAME,
        VK_KHR_PRESENT_WAIT_EXTENSION_NAME,
        VK_KHR_MAINTENANCE_5_EXTENSION_NAME,
        VK_EXT_MESH_SHADER_EXTENSION_NAME,
        VK_EXT_MUTABLE_DESCRIPTOR_TYPE_EXTENSION_NAME
    };

    createInfo.ppEnabledExtensionNames = extensions;
    createInfo.enabledExtensionCount = ARRAYSIZE(extensions);

    VkPhysicalDeviceFeatures2 features = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
    features.features.shaderInt16 = true;
    features.features.shaderInt64 = true;
    features.features.pipelineStatisticsQuery = true;

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

    if (VulkanDebugLayerEnabled)
    {
        features12.bufferDeviceAddressCaptureReplay = true;
    }

    VkPhysicalDeviceVulkan13Features features13 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
    features13.maintenance4 = true;
    features13.synchronization2 = true;
    features13.dynamicRendering = true;

    VkPhysicalDeviceMaintenance5FeaturesKHR maintenance5 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_5_FEATURES_KHR };
    maintenance5.maintenance5 = true;

    VkPhysicalDeviceMeshShaderFeaturesEXT meshFeatures = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT };
    meshFeatures.meshShader = true;
    meshFeatures.taskShader = true;
    meshFeatures.meshShaderQueries = true;

    VkPhysicalDeviceMutableDescriptorTypeFeaturesEXT mutableDescriptorFeatures = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MUTABLE_DESCRIPTOR_TYPE_FEATURES_EXT };
    mutableDescriptorFeatures.mutableDescriptorType = true;

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
    meshFeatures.pNext = &maintenance5;

    // TODO: Test Features
    maintenance5.pNext = &mutableDescriptorFeatures;

    VkDevice device = nullptr;
    AssertIfFailedReturnNullHandle(vkCreateDevice(physicalDevice, &createInfo, nullptr, &device));
    volkLoadDevice(device);

    auto handle = SystemAddDataPoolItem(vulkanGraphicsDevicePool, {
        .Device = device,
    }); 

    SystemAddDataPoolItemFull(vulkanGraphicsDevicePool, handle, {
        .PhysicalDevice = physicalDevice,
        .DeviceProperties = deviceProperties,
        .DeviceMemoryProperties = deviceMemoryProperties,
        .RenderCommandQueueIndex = renderCommandQueueIndex,
        .ComputeCommandQueueIndex = computeCommandQueueIndex,
        .CopyCommandQueueIndex = copyCommandQueueIndex,
        .GpuMemoryTypeIndex = (uint32_t)gpuMemoryTypeIndex,
        .GpuUploadMemoryTypeIndex = (uint32_t)gpuUploadMemoryTypeIndex,
        .ReadBackMemoryTypeIndex = (uint32_t)readBackMemoryTypeIndex
    });

    CreateVulkanPipelineLayout(handle);

    return handle;
}

void VulkanFreeGraphicsDevice(ElemGraphicsDevice graphicsDevice)
{
    SystemAssert(graphicsDevice != ELEM_HANDLE_NULL);

    auto graphicsDeviceData = GetVulkanGraphicsDeviceData(graphicsDevice);
    SystemAssert(graphicsDeviceData);

    auto graphicsDeviceDataFull = GetVulkanGraphicsDeviceDataFull(graphicsDevice);
    SystemAssert(graphicsDeviceDataFull);

    vkDestroyDescriptorSetLayout(graphicsDeviceData->Device, graphicsDeviceDataFull->ResourceDescriptorSetLayout, nullptr);
    vkDestroyPipelineLayout(graphicsDeviceData->Device, graphicsDeviceData->PipelineLayout, nullptr);
    vkDestroyDevice(graphicsDeviceData->Device, nullptr);
        
    SystemRemoveDataPoolItem(vulkanGraphicsDevicePool, graphicsDevice);
    SystemLogDebugMessage(ElemLogMessageCategory_Graphics, "Releasing Vulkan");
}

ElemGraphicsDeviceInfo VulkanGetGraphicsDeviceInfo(ElemGraphicsDevice graphicsDevice)
{
    SystemAssert(graphicsDevice != ELEM_HANDLE_NULL);

    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto graphicsDeviceDataFull = GetVulkanGraphicsDeviceDataFull(graphicsDevice);
    SystemAssert(graphicsDeviceDataFull);

    return VulkanConstructGraphicsDeviceInfo(stackMemoryArena, graphicsDeviceDataFull->DeviceProperties, graphicsDeviceDataFull->DeviceMemoryProperties);
}
