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
    spirv_cross::CompilerMSL compiler((uint32_t*)shaderCode, shaderCodeSize / 4);

    spirv_cross::CompilerMSL::Options options;
    options.set_msl_version(3, 0, 0);

    compiler.set_msl_options(options);
    auto metalCode = compiler.compile();

    // TODO: Check for errors
    auto outputShaderData = new uint8_t[metalCode.length()];
    memcpy(outputShaderData, metalCode.c_str(), metalCode.length());

    printf("MetalCode: %s\n", metalCode.c_str());

    ShaderCompilerResult result = {};

    result.IsSuccess = true;
    result.Stage = shaderStage;
    result.EntryPoint = entryPoint;
    result.ShaderData = outputShaderData;
    result.ShaderDataCount = metalCode.length();
    result.LogEntries = nullptr;
    result.LogEntryCount = 0;

    return result;
}