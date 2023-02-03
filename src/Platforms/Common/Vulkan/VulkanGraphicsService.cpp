#ifdef _WINDOWS
#include "WindowsCommon.h"
#endif

#include "VulkanGraphicsService.h"

void VulkanGraphicsService::GetAvailableGraphicsDevices(void* graphicsDevices, int* count)
{
    count = 0;
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
    result.GraphicsApiName = ConvertWStringToUtf8(L"API");
    result.DriverVersion = ConvertWStringToUtf8(L"1.0");

    return result;
}