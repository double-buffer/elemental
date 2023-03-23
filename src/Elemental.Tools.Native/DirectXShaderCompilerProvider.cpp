#include "DirectXShaderCompilerProvider.h"

ShaderLanguage DirectXShaderCompilerProvider::GetShaderLanguage()
{
    return ShaderLanguage_Hlsl;
}

void DirectXShaderCompilerProvider::GetTargetShaderLanguages(ShaderLanguage* targetLanguages, int *targetLanguagesCount)
{
    int index = 0;

    targetLanguages[index++] = ShaderLanguage_Dxil;
    targetLanguages[index++] = ShaderLanguage_Spirv;

    *targetLanguagesCount = index;
}

bool DirectXShaderCompilerProvider::IsCompilerInstalled()
{
    return true;
}