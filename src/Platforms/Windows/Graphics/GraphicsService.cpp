#include "WindowsCommon.h"
#include "GraphicsService.h"
#include "Direct3D12/Direct3D12GraphicsService.h"

// TODO: To change
BaseGraphicsService *graphicsService = (BaseGraphicsService*)new Direct3D12GraphicsService();

DllExport void* Native_CreateGraphicsDevice(GraphicsDiagnostics graphicsDiagnostics)
{
    return graphicsService->CreateGraphicsDevice(graphicsDiagnostics);
}

DllExport void Native_DeleteGraphicsDevice(void* graphicsDevicePointer)
{
    graphicsService->DeleteGraphicsDevice(graphicsDevicePointer);
}

DllExport GraphicsDeviceInfo Native_GetGraphicsDeviceInfo(void* graphicsDevicePointer)
{
    return graphicsService->GetGraphicsDeviceInfo(graphicsDevicePointer);
}