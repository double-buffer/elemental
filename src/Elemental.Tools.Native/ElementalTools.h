#pragma once
#include "SystemFunctions.h"
#include "../Platforms/Common/ElementalCommon.h"

enum ShaderLanguage
{
    ShaderLanguage_Unknown = 0,
    ShaderLanguage_Hlsl = 1,
    ShaderLanguage_Msl = 2,
    ShaderLanguage_Dxil = 3,
    ShaderLanguage_Spirv = 4,
    ShaderLanguage_MetalIR = 5
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
    enum ShaderStage Stage;
    uint8_t* EntryPoint;
    uint8_t* ShaderData;
    int32_t ShaderDataCount;
    ShaderCompilerLogEntry* LogEntries;
    int32_t LogEntryCount;
};

ShaderCompilerResult CreateErrorResult(ShaderStage stage, uint8_t* entryPoint, uint8_t* message)
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