#pragma once
#include "ElementalTools.h"
#include "ShaderCompilerProvider.h"

class DirectXShaderCompilerProvider : ShaderCompilerProvider
{
public:
    DirectXShaderCompilerProvider(){};

    ShaderLanguage GetShaderLanguage() override;
    void GetTargetShaderLanguages(ShaderLanguage* targetLanguages, int *targetLanguagesCount) override;

    bool IsCompilerInstalled() override;
};