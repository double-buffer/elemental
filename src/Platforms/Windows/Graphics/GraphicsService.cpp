#include "WindowsCommon.h"
#include "GraphicsService.h"
#include "Direct3D12/Direct3D12GraphicsService.h"

// TODO: To change
BaseGraphicsService *graphicsService = (BaseGraphicsService*)new Direct3D12GraphicsService();

DllExport void Native_GraphicsServiceInit()
{
    printf("Graphics INIT\n");
}

DllExport void Native_GraphicsServiceDispose()
{
    printf("Graphics Dispose\n");
}

DllExport void* Native_CreateGraphicsDevice(GraphicsDeviceOptions options)
{
    return graphicsService->CreateGraphicsDevice(options);
}

DllExport void Native_DeleteGraphicsDevice(void* graphicsDevicePointer)
{
    graphicsService->DeleteGraphicsDevice(graphicsDevicePointer);
}

DllExport GraphicsDeviceInfo Native_GetGraphicsDeviceInfo(void* graphicsDevicePointer)
{
    return graphicsService->GetGraphicsDeviceInfo(graphicsDevicePointer);
}