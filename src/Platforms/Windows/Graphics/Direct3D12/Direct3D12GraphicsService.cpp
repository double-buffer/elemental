#include "WindowsCommon.h"
#include "Direct3D12GraphicsService.h"

void* Direct3D12GraphicsService::CreateGraphicsDevice(GraphicsDiagnostics graphicsDiagnostics)
{
    printf("Create Device\n");

    if (graphicsDiagnostics == GraphicsDiagnostics_Debug)
    {
        printf("DEBUG ON\n");
    }

    auto test = new int();
    return test;
}

void Direct3D12GraphicsService::DeleteGraphicsDevice(void* graphicsDevicePointer)
{
}

GraphicsDeviceInfo Direct3D12GraphicsService::GetGraphicsDeviceInfo(void* graphicsDevicePointer)
{
    auto result = GraphicsDeviceInfo();
    result.DeviceName = ConvertWStringToUtf8(L"Windows Device éééé");
    result.GraphicsApiName = ConvertWStringToUtf8(L"API");
    result.DriverVersion = ConvertWStringToUtf8(L"1.0");

    return result;
}