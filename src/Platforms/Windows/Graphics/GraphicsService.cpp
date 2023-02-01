#include "WindowsCommon.h"
#include "Direct3D12/Direct3D12GraphicsService.h"
#include "../../Common/BaseGraphicsObject.h"
#include "../../Common/Vulkan/VulkanGraphicsService.h"

DllExport void Native_GraphicsServiceInit()
{
}

DllExport void Native_GraphicsServiceDispose()
{
}

DllExport void* Native_CreateGraphicsDevice(GraphicsDeviceOptions options)
{
    BaseGraphicsService* graphicsService;

    if (options.UseVulkan)
    {
        graphicsService = (BaseGraphicsService*)new VulkanGraphicsService();
    }
    else
    {
        graphicsService = (BaseGraphicsService*)new Direct3D12GraphicsService();
    }

    return graphicsService->CreateGraphicsDevice(options);
}

DllExport void Native_DeleteGraphicsDevice(void* graphicsDevicePointer)
{
    auto graphicsService = ((BaseGraphicsObject*)graphicsDevicePointer)->GraphicsService;
    graphicsService->DeleteGraphicsDevice(graphicsDevicePointer);
}

DllExport GraphicsDeviceInfo Native_GetGraphicsDeviceInfo(void* graphicsDevicePointer)
{
    auto graphicsService = ((BaseGraphicsObject*)graphicsDevicePointer)->GraphicsService;
    return graphicsService->GetGraphicsDeviceInfo(graphicsDevicePointer);
}