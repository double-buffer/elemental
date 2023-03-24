#include "SpirvCrossShaderCompilerProvider.h"

ShaderLanguage SpirvCrossShaderCompilerProvider::GetShaderLanguage()
{
    return ShaderLanguage_Spirv;
}

void SpirvCrossShaderCompilerProvider::GetTargetShaderLanguages(ShaderLanguage* targetLanguages, int* targetLanguagesCount)
{
    int32_t index = 0;

    targetLanguages[index++] = ShaderLanguage_Msl;
    *targetLanguagesCount = index;
}

bool SpirvCrossShaderCompilerProvider::IsCompilerInstalled()
{
    return true;
}
    
ShaderCompilerResult SpirvCrossShaderCompilerProvider::CompileShader(uint8_t* shaderCode, uint32_t shaderCodeSize, ToolsShaderStage shaderStage, uint8_t* entryPoint, ShaderLanguage shaderLanguage, ToolsGraphicsApi graphicsApi)
{
    return CreateErrorResult(shaderStage, entryPoint, ConvertWStringToUtf8(L"This is a test from SPIRV Cross..."));
}