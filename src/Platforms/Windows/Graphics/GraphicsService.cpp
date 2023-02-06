#include "WindowsCommon.h"
#include "Direct3D12/Direct3D12GraphicsService.h"
#include "../../Common/BaseGraphicsObject.h"
#include "../../Common/BaseGraphicsService.h"
#include "../../Common/Vulkan/VulkanGraphicsService.h"

DllExport void Native_GetAvailableGraphicsDevices(GraphicsDeviceInfo* graphicsDevices, int* count)
{
    auto direct3D12Service = Direct3D12GraphicsService();
    direct3D12Service.GetAvailableGraphicsDevices(graphicsDevices, count);

    // TODO: Combine vulkan devices
}

DllExport void* Native_CreateGraphicsDevice(GraphicsDeviceOptions options)
{
    GraphicsDeviceInfo availableDevices[50];
    int availableDeviceCount;

    Native_GetAvailableGraphicsDevices(availableDevices, &availableDeviceCount);

    if (availableDeviceCount == 0)
    {
        return nullptr;
    }

    auto selectedDevice = availableDevices[0];

    if (options.DeviceId != 0)
    {
        for (int i = 0; i < availableDeviceCount; i++)
        {
            if (availableDevices[i].DeviceId == options.DeviceId)
            {
                selectedDevice = availableDevices[i];
                break;
            }
        }
    }

    BaseGraphicsService* graphicsService;

    if (selectedDevice.GraphicsApi == GraphicsApi_Vulkan)
    {
        graphicsService = (BaseGraphicsService*)new VulkanGraphicsService();
    }
    else
    {
        graphicsService = (BaseGraphicsService*)new Direct3D12GraphicsService();
    }

    return graphicsService->CreateGraphicsDevice(options);
}

DllExport void Native_FreeGraphicsDevice(void* graphicsDevicePointer)
{
    auto graphicsService = ((BaseGraphicsObject*)graphicsDevicePointer)->GraphicsService;
    graphicsService->FreeGraphicsDevice(graphicsDevicePointer);
}

DllExport GraphicsDeviceInfo Native_GetGraphicsDeviceInfo(void* graphicsDevicePointer)
{
    auto graphicsService = ((BaseGraphicsObject*)graphicsDevicePointer)->GraphicsService;
    return graphicsService->GetGraphicsDeviceInfo(graphicsDevicePointer);
}