#include "MetalShaderCompilerProvider.h"

ShaderLanguage MetalShaderCompilerProvider::GetShaderLanguage()
{
    return ShaderLanguage_Msl;
}

void MetalShaderCompilerProvider::GetTargetShaderLanguages(ShaderLanguage* targetLanguages, int* targetLanguagesCount)
{
    int32_t index = 0;

    targetLanguages[index++] = ShaderLanguage_MetalIR;
    *targetLanguagesCount = index;
}

bool MetalShaderCompilerProvider::IsCompilerInstalled()
{
    return true;
}
    
ShaderCompilerResult MetalShaderCompilerProvider::CompileShader(uint8_t* shaderCode, uint32_t shaderCodeSize, ToolsShaderStage shaderStage, uint8_t* entryPoint, ShaderLanguage shaderLanguage, ToolsGraphicsApi graphicsApi)
{
    return CreateErrorResult(shaderStage, entryPoint, ConvertWStringToUtf8(L"This is a test from Metal..."));
}