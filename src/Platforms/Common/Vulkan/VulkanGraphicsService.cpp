#ifdef _WINDOWS
#include "WindowsCommon.h"
#endif

#include "VulkanGraphicsService.h"

VulkanGraphicsService::VulkanGraphicsService(GraphicsServicesOptions options)
{
    InitSdk(options.GraphicsDiagnostics == GraphicsDiagnostics_Debug);
}

void VulkanGraphicsService::GetAvailableGraphicsDevices(GraphicsDeviceInfo* graphicsDevices, int* count)
{
    graphicsDevices[(*count)++] = ConstructGraphicsDeviceInfo(0);
}

void* VulkanGraphicsService::CreateGraphicsDevice(GraphicsDeviceOptions options)
{
    printf("Create Vulkan Device\n");

    // TODO: Important !!
    //volkLoadDevice(this->graphicsDevice);
    
    auto graphicsDevice = new BaseGraphicsObject(this);
    return graphicsDevice;
}

void VulkanGraphicsService::FreeGraphicsDevice(void* graphicsDevicePointer)
{
}

GraphicsDeviceInfo VulkanGraphicsService::GetGraphicsDeviceInfo(void* graphicsDevicePointer)
{
    auto result = GraphicsDeviceInfo();
    result.DeviceName = ConvertWStringToUtf8(L"Vulkan Windows Device éééé");
    result.GraphicsApi = GraphicsApi_Vulkan;

    return result;
}

GraphicsDeviceInfo VulkanGraphicsService::ConstructGraphicsDeviceInfo(int adapterDescription)
{
    auto result = GraphicsDeviceInfo();
    result.DeviceName = ConvertWStringToUtf8(L"Test Vulkan Device");
    result.GraphicsApi = GraphicsApi_Vulkan;
    result.DeviceId = 2828;
    result.AvailableMemory = 12345;

    return result;
}

void VulkanGraphicsService::InitSdk(bool enableDebugDiagnostics)
{
    AssertIfFailed(volkInitialize());

    VkInstance instance = {};

    VkApplicationInfo appInfo = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
    appInfo.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo createInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };

    createInfo.pApplicationInfo = &appInfo;

    if (enableDebugDiagnostics)
    {
        printf("VULKAN DEBUG!\n");

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
            VK_KHR_WIN32_SURFACE_EXTENSION_NAME // TODO: Add a preprocessor check here
        };
        
        createInfo.ppEnabledExtensionNames = extensions;
	    createInfo.enabledExtensionCount = ARRAYSIZE(extensions);
        
        AssertIfFailed(vkCreateInstance(&createInfo, nullptr, &instance));
    }

    else
    {
        const char* extensions[] =
        {
            VK_KHR_SURFACE_EXTENSION_NAME,
            VK_KHR_WIN32_SURFACE_EXTENSION_NAME // TODO: Add a preprocessor check here
        };
        
        createInfo.ppEnabledExtensionNames = extensions;
	    createInfo.enabledExtensionCount = ARRAYSIZE(extensions);
    
        AssertIfFailed(vkCreateInstance(&createInfo, nullptr, &instance));
    }

    volkLoadInstance(instance);

    uint32_t deviceCount = 16;
    VkPhysicalDevice devices[16];

    AssertIfFailed(vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr));
    AssertIfFailed(vkEnumeratePhysicalDevices(instance, &deviceCount, devices));

    for (uint32_t i = 0; i < deviceCount; i++)
    {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(devices[i], &deviceProperties);

        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceFeatures(devices[i], &deviceFeatures);

        VkPhysicalDeviceMeshShaderFeaturesNV meshShaderFeatures = {};
        meshShaderFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_NV;

        VkPhysicalDeviceFeatures2 features2 = {};
        features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR;
        features2.pNext = &meshShaderFeatures;

        vkGetPhysicalDeviceFeatures2(devices[i], &features2);

        if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && meshShaderFeatures.meshShader && meshShaderFeatures.taskShader)
        {
            char* currentDeviceName = deviceProperties.deviceName;
            auto deviceName = std::wstring(currentDeviceName, currentDeviceName + strlen(currentDeviceName));
            //this->deviceName += wstring(L" (Vulkan " + to_wstring(VK_API_VERSION_MAJOR(VK_HEADER_VERSION_COMPLETE)) + L"." + to_wstring(VK_API_VERSION_MINOR(VK_HEADER_VERSION_COMPLETE)) + L"." + to_wstring(VK_API_VERSION_PATCH(VK_HEADER_VERSION_COMPLETE)) + L")");

            printf("%ls\n", deviceName.c_str());
        }
    }
}