#pragma once
#include "ElementalTools.h"
#include "ShaderCompilerProvider.h"

#include "dxcapi.h"

class DirectXShaderCompilerProvider : ShaderCompilerProvider
{
public:
    DirectXShaderCompilerProvider();
    ~DirectXShaderCompilerProvider() override;

    ShaderLanguage GetShaderLanguage() override;
    void GetTargetShaderLanguages(ShaderLanguage* targetLanguages, int* targetLanguagesCount) override;

    bool IsCompilerInstalled() override;
    ShaderCompilerResult CompileShader(uint8_t* shaderCode, uint32_t shaderCodeSize, ToolsShaderStage shaderStage, uint8_t* entryPoint, ShaderLanguage shaderLanguage, ToolsGraphicsApi graphicsApi) override;

private:
    HMODULE _dxcompilerDll = nullptr;
    DxcCreateInstanceProc _createInstanceFunc = nullptr;
};