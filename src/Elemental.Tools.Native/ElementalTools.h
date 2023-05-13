#pragma once
#include "ElementalCommon.h"

typedef struct 
{
    LogMessageHandlerPtr LogMessageHandler;
} ShaderCompilerOptions;

typedef enum
{
    ShaderLanguage_Unknown = 0,
    ShaderLanguage_Hlsl = 1,
    ShaderLanguage_Msl = 2,
    ShaderLanguage_Dxil = 3,
    ShaderLanguage_Spirv = 4,
    ShaderLanguage_MetalIR = 5
} ShaderLanguage;

typedef enum
{
    ShaderCompilerLogEntryType_Message = 0,
    ShaderCompilerLogEntryType_Warning = 1,
    ShaderCompilerLogEntryType_Error = 2
} ShaderCompilerLogEntryType;

typedef struct
{
    ShaderCompilerLogEntryType Type;
    const uint8_t* Message;
} ShaderCompilerLogEntry;

typedef struct
{
    bool DebugMode;
} ShaderCompilationOptions;

typedef struct
{
    uint8_t* ShaderCode;
    ShaderStage Stage;
    uint8_t* EntryPoint;
    ShaderLanguage ShaderLanguage;
} ShaderCompilerInput;

typedef struct
{
    bool IsSuccess;
    ShaderStage Stage;
    const uint8_t* EntryPoint;
    uint8_t* ShaderData;
    uint32_t ShaderDataCount;
    ShaderCompilerLogEntry* LogEntries;
    uint32_t LogEntryCount;
    ShaderMetaData* MetaData;
    uint32_t MetaDataCount;
} ShaderCompilerResult;

const ShaderCompilerResult CreateErrorResult(ShaderStage stage, const uint8_t* entryPoint, const uint8_t* message)
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