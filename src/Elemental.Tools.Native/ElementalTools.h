#pragma once

#if _WINDOWS
#define DllExport extern "C" __declspec(dllexport)
#else
#define DllExport extern "C" __attribute__((visibility("default"))) 
#endif

#include <stdio.h>
#include <stdint.h>

enum ToolsGraphicsApi
{
    ToolsGraphicsApi_Unknown = 0,
    ToolsGraphicsApi_Direct3D12 = 1,
    ToolsGraphicsApi_Vulkan = 2,
    ToolsGraphicsApi_Metal = 3
};

enum ShaderLanguage
{
    ShaderLanguage_Unknown = 0,
    ShaderLanguage_Hlsl = 1,
    ShaderLanguage_Msl = 2,
    ShaderLanguage_Dxil = 3,
    ShaderLanguage_Spirv = 4,
    ShaderLanguage_MetalIR = 5
};