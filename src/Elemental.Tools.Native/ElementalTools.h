#pragma once

#if _WINDOWS
#define DllExport extern "C" __declspec(dllexport)
#include <windows.h>
#else
#define DllExport extern "C" __attribute__((visibility("default"))) 
#endif

#include <stdio.h>
#include <stdint.h>
#include <string>

uint8_t* ConvertWStringToUtf8(const std::wstring &source)
{
    char *destination = new char[source.length() + 1];
    destination[source.length()] = '\0';
    WideCharToMultiByte(CP_ACP, 0, source.c_str(), -1, destination, (int)source.length(), NULL, NULL);

    return (uint8_t*)destination;
}

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

enum ToolsShaderStage
{
    ToolsShaderStage_None = 0,
    ToolsShaderStage_AmplificationShader = 1,
    ToolsShaderStage_MeshShader = 2,
    ToolsShaderStage_PixelShader = 3
};

enum ShaderCompilerLogEntryType
{
    ShaderCompilerLogEntryType_Message = 0,
    ShaderCompilerLogEntryType_Warning = 1,
    ShaderCompilerLogEntryType_Error = 2
};

struct ShaderCompilerLogEntry
{
    enum ShaderCompilerLogEntryType Type;
    uint8_t* Message;
};

struct ShaderCompilerResult
{
    bool IsSuccess;
    enum ToolsShaderStage Stage;
    uint8_t* EntryPoint;
    uint8_t* ShaderData;
    int32_t ShaderDataCount;
    ShaderCompilerLogEntry* LogEntries;
    int32_t LogEntryCount;
};

ShaderCompilerResult CreateErrorResult(ToolsShaderStage stage, uint8_t* entryPoint, uint8_t* message)
{
    ShaderCompilerResult result = {};

    result.IsSuccess = false;
    result.Stage = stage;
    result.EntryPoint = entryPoint;
    result.LogEntries = new ShaderCompilerLogEntry[1];
    result.LogEntries[0].Type = ShaderCompilerLogEntryType_Error;
    result.LogEntries[0].Message = message;
    result.LogEntryCount = 1;

    return result;
}