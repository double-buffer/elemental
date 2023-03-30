#pragma once
#include "SystemFunctions.h"
#include "../Platforms/Common/ElementalCommon.h"

template<typename T>
struct Span
{
    Span()
    {
        Pointer = nullptr;
        Length = 0;
    }

    Span(T* pointer, uint32_t length) : Pointer(pointer), Length(length)
    {
    }

    T* Pointer;
    uint32_t Length;

    bool IsEmpty()
    {
        return Length == 0;
    }

    static Span<T> Empty()
    {
        return Span<T>();
    }
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

struct ShaderCompilationOptions
{
    bool DebugMode;
};

struct ShaderCompilerInput
{
    uint8_t* ShaderCode;
    enum ShaderStage Stage;
    uint8_t* EntryPoint;
    enum ShaderLanguage ShaderLanguage;
};

struct ShaderCompilerResult
{
    bool IsSuccess;
    enum ShaderStage Stage;
    uint8_t* EntryPoint;
    uint8_t* ShaderData;
    int32_t ShaderDataCount;
    struct ShaderCompilerLogEntry* LogEntries;
    int32_t LogEntryCount;
    struct ShaderMetaData* MetaData;
    int32_t MetaDataCount;
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