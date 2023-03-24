#pragma once
#include "ElementalTools.h"
#include "ShaderCompilerProvider.h"

class MetalShaderCompilerProvider : ShaderCompilerProvider
{
public:
    MetalShaderCompilerProvider() {};

    ShaderLanguage GetShaderLanguage() override;
    void GetTargetShaderLanguages(ShaderLanguage* targetLanguages, int* targetLanguagesCount) override;

    bool IsCompilerInstalled() override;
    ShaderCompilerResult CompileShader(uint8_t* shaderCode, uint32_t shaderCodeSize, ToolsShaderStage shaderStage, uint8_t* entryPoint, ShaderLanguage shaderLanguage, ToolsGraphicsApi graphicsApi) override;
};