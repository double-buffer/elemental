#pragma once
#include "SystemMemory.h"
#include "ElementalTools.h"

class ShaderCompilerProvider
{
public:
    ShaderCompilerProvider() {};
    virtual ~ShaderCompilerProvider() {};

    virtual ShaderLanguage GetShaderLanguage() = 0;
    virtual void GetTargetShaderLanguages(ShaderLanguage* targetLanguages, int* targetLanguagesCount) = 0;

    virtual bool IsCompilerInstalled() = 0;
    virtual Span<uint8_t> CompileShader(MemoryArena* memoryArena, std::vector<ShaderCompilerLogEntry>& logList, std::vector<ShaderMetaData>& metaDataList, Span<uint8_t> shaderCode, ShaderStage shaderStage, uint8_t* entryPoint, ShaderLanguage shaderLanguage, GraphicsApi graphicsApi, ShaderCompilationOptions* options) = 0;
};
