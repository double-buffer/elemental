#include "DirectXShaderCompilerProvider.h"

ShaderLanguage DirectXShaderCompilerProvider::GetShaderLanguage()
{
    return ShaderLanguage_Hlsl;
}

void DirectXShaderCompilerProvider::GetTargetShaderLanguages(ShaderLanguage* targetLanguages, int* targetLanguagesCount)
{
    int32_t index = 0;

    targetLanguages[index++] = ShaderLanguage_Dxil;
    targetLanguages[index++] = ShaderLanguage_Spirv;

    *targetLanguagesCount = index;
}

bool DirectXShaderCompilerProvider::IsCompilerInstalled()
{
    return true;
}
    
ShaderCompilerResult DirectXShaderCompilerProvider::CompileShader(uint8_t* shaderCode, uint32_t shaderCodeSize, ToolsShaderStage shaderStage, uint8_t* entryPoint, ShaderLanguage shaderLanguage, ToolsGraphicsApi graphicsApi)
{
    return CreateErrorResult(shaderStage, entryPoint, ConvertWStringToUtf8(L"This is a test from DXC..."));
}