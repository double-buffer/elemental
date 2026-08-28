#include "VulkanGraphicsDevice.h"
#include "VulkanConfig.h"
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

    if (SystemFindSubString(pMessage, "BestPractices-PushConstants") != -1)
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
    SystemAssert(volkGetInstanceVersion() >= VK_API_VERSION_1_4);

    VkApplicationInfo appInfo = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
    appInfo.apiVersion = VK_API_VERSION_1_4;

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

            // Test B: keep VK_LAYER_KHRONOS_validation enabled without additional validation features.
            // This isolates the base validation layer from Best Practices / Synchronization Validation.
            createInfo.pNext = nullptr;

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
        debugCreateInfo.flags = VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
        debugCreateInfo.pfnCallback = VulkanDebugReportCallback;

        AssertIfFailed(vkCreateDebugReportCallbackEXT(VulkanInstance, &debugCreateInfo, 0, &vulkanDebugCallback));
    }
}
