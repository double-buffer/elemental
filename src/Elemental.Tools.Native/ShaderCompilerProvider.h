#pragma once
#include "ElementalTools.h"

class ShaderCompilerProvider
{
public:
    virtual ShaderLanguage GetShaderLanguage() = 0;
    virtual void GetTargetShaderLanguages(ShaderLanguage* targetLanguages, int* targetLanguagesCount) = 0;

    virtual bool IsCompilerInstalled() = 0;
    //ShaderCompilerResult CompileShader(ReadOnlySpan<byte> shaderCode, ToolsShaderStage shaderStage, string entryPoint, ShaderLanguage shaderLanguage, ToolsGraphicsApi graphicsApi);
};