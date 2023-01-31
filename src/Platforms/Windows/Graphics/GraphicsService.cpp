#include "WindowsCommon.h"
#include "GraphicsService.h"

DllExport void* Native_CreateGraphicsDevice(GraphicsDiagnostics graphicsDiagnostics)
{
    printf("Create Device\n");

    if (graphicsDiagnostics == GraphicsDiagnostics_Debug)
    {
        printf("DEBUG ON\n");
    }

    auto test = new int();
    return test;
}

DllExport void Native_DeleteGraphicsDevice(void* graphicsDevicePointer)
{
}

DllExport GraphicsDeviceInfo Native_GetGraphicsDeviceInfo(void* graphicsDevicePointer)
{
    auto result = GraphicsDeviceInfo();
    result.DeviceName = ConvertWStringToUtf8(L"Windows Device éééé");
    result.GraphicsApiName = ConvertWStringToUtf8(L"API");
    result.DriverVersion = ConvertWStringToUtf8(L"1.0");

    return result;
}