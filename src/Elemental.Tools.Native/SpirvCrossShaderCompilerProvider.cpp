#include "PrecompiledHeader.h"
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
    
Span<uint8_t> SpirvCrossShaderCompilerProvider::CompileShader(std::vector<ShaderCompilerLogEntry>& logList, std::vector<ShaderMetaData>& metaDataList, Span<uint8_t> shaderCode, ShaderStage shaderStage, uint8_t* entryPoint, ShaderLanguage shaderLanguage, GraphicsApi graphicsApi, ShaderCompilationOptions* options)
{
    // TODO: Check target api
    spirv_cross::CompilerMSL compiler((uint32_t*)shaderCode.Pointer, shaderCode.Length / 4);

    spirv_cross::CompilerMSL::Options mslOptions;
    mslOptions.set_msl_version(3, 0, 0);

    compiler.set_msl_options(mslOptions);
    auto metalCode = compiler.compile();

    // TODO: Check for errors
    auto outputShaderData = new uint8_t[metalCode.length()];
    memcpy(outputShaderData, metalCode.c_str(), metalCode.length());

    printf("MetalCode: %s\n", metalCode.c_str());
    return Span<uint8_t>(outputShaderData, (uint32_t)metalCode.length());
}