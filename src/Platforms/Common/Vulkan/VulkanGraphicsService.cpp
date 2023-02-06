#ifdef _WINDOWS
#include "WindowsCommon.h"
#endif

#include "VulkanGraphicsService.h"

void VulkanGraphicsService::GetAvailableGraphicsDevices(GraphicsDeviceInfo* graphicsDevices, int* count)
{
    graphicsDevices[(*count)++] = ConstructGraphicsDeviceInfo(0);
}

void* VulkanGraphicsService::CreateGraphicsDevice(GraphicsDeviceOptions options)
{
    printf("Create Vulkan Device\n");
    
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