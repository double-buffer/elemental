#include "WindowsCommon.h"
#include "Direct3D12/Direct3D12GraphicsService.h"
#include "../../Common/BaseGraphicsObject.h"
#include "../../Common/BaseGraphicsService.h"
#include "../../Common/Vulkan/VulkanGraphicsService.h"

static Direct3D12GraphicsService* _globalDirect3D12GraphicsService; 
static VulkanGraphicsService* _globalVulkanGraphicsService; 

DllExport void Native_InitGraphicsService(GraphicsServiceOptions options)
{
    _globalDirect3D12GraphicsService = new Direct3D12GraphicsService(options);
    _globalVulkanGraphicsService = new VulkanGraphicsService(options);
}

DllExport void Native_FreeGraphicsService()
{
    delete _globalDirect3D12GraphicsService;
    delete _globalVulkanGraphicsService;
}

DllExport void Native_GetAvailableGraphicsDevices(GraphicsDeviceInfo* graphicsDevices, int* count)
{
    (*count) = 0;

    _globalDirect3D12GraphicsService->GetAvailableGraphicsDevices(graphicsDevices, count);
    _globalVulkanGraphicsService->GetAvailableGraphicsDevices(graphicsDevices, count);
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

    options.DeviceId = selectedDevice.DeviceId;

    BaseGraphicsService* graphicsService;

    if (selectedDevice.GraphicsApi == GraphicsApi_Vulkan)
    {
        graphicsService = (BaseGraphicsService*)_globalVulkanGraphicsService;
    }
    else
    {
        graphicsService = (BaseGraphicsService*)_globalDirect3D12GraphicsService;
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