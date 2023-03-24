#pragma once
#include "ElementalTools.h"

class ShaderCompilerProvider
{
public:
    ShaderCompilerProvider() {};
    virtual ~ShaderCompilerProvider() {};

    virtual ShaderLanguage GetShaderLanguage() = 0;
    virtual void GetTargetShaderLanguages(ShaderLanguage* targetLanguages, int* targetLanguagesCount) = 0;

    virtual bool IsCompilerInstalled() = 0;
    virtual ShaderCompilerResult CompileShader(uint8_t* shaderCode, uint32_t shaderCodeSize, ToolsShaderStage shaderStage, uint8_t* entryPoint, ShaderLanguage shaderLanguage, ToolsGraphicsApi graphicsApi) = 0;
};