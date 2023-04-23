#pragma once
#include "PreCompiledHeader.h"
#include "ElementalTools.h"
#include "ShaderCompilerProvider.h"

class SpirvCrossShaderCompilerProvider : ShaderCompilerProvider
{
public:
    SpirvCrossShaderCompilerProvider() {};

    ShaderLanguage GetShaderLanguage() override;
    void GetTargetShaderLanguages(ShaderLanguage* targetLanguages, int* targetLanguagesCount) override;

    bool IsCompilerInstalled() override;
    Span<uint8_t> CompileShader(std::vector<ShaderCompilerLogEntry>& logList, std::vector<ShaderMetaData>& metaDataList, Span<uint8_t> shaderCode, ShaderStage shaderStage, uint8_t* entryPoint, ShaderLanguage shaderLanguage, GraphicsApi graphicsApi, ShaderCompilationOptions* options) override;
};