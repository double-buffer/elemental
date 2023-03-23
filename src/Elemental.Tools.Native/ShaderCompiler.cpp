#include "ElementalTools.h"
#include "ShaderCompilerProvider.h"
#include "DirectXShaderCompilerProvider.h"

// TODO: Do we keep std datastructures? Perf is not critical in the tools
#include <vector>
#include <unordered_map>

std::vector<ShaderCompilerProvider*> _shaderCompilerProviders;
std::unordered_map<ToolsGraphicsApi, ShaderLanguage> _platformTargetLanguages;

void RegisterShaderCompilerProvider(ShaderCompilerProvider* shaderCompilerProvider)
{
    if (!shaderCompilerProvider->IsCompilerInstalled())
    {
        delete shaderCompilerProvider;
        return;
    }
    
    _shaderCompilerProviders.push_back(shaderCompilerProvider);
}

ShaderCompilerProvider* FindShaderCompiler(ShaderLanguage targetShaderLanguage)
{
    for (auto i = 0; i < _shaderCompilerProviders.size(); i++)
    {
        auto shaderCompilerProvider = _shaderCompilerProviders[i];

        ShaderLanguage targetLanguages[10];
        int targetLanguageCount = 0;

        shaderCompilerProvider->GetTargetShaderLanguages(targetLanguages, &targetLanguageCount);

        for (auto j = 0; j < targetLanguageCount; j++)
        {
            auto providerTargetShaderLanguage = targetLanguages[j];

            if (providerTargetShaderLanguage == targetShaderLanguage)
            {
                return shaderCompilerProvider;
            }
        }
    }

    return nullptr;
}

int FindShaderCompilersChain(ShaderCompilerProvider** shaderCompilers, int32_t currentLevel, ShaderLanguage sourceLanguage, ShaderLanguage targetLanguage)
{
    auto compiler = FindShaderCompiler(targetLanguage);

    if (compiler == nullptr)
    {
        return 0;
    }

    shaderCompilers[currentLevel] = compiler;

    if (compiler->GetShaderLanguage() == sourceLanguage)
    {
        return 1;
    }
    else
    {
        auto result = FindShaderCompilersChain(shaderCompilers, currentLevel + 1, sourceLanguage, compiler->GetShaderLanguage());

        if (result > 0)
        {
            return result + 1;
        }

        return 0;
    }
}

DllExport void Native_InitShaderCompiler()
{
    _platformTargetLanguages[ToolsGraphicsApi_Direct3D12] = ShaderLanguage_Dxil;
    _platformTargetLanguages[ToolsGraphicsApi_Vulkan] = ShaderLanguage_Spirv;
    _platformTargetLanguages[ToolsGraphicsApi_Metal] = ShaderLanguage_MetalIR;

    RegisterShaderCompilerProvider((ShaderCompilerProvider*)new DirectXShaderCompilerProvider());
}
    
DllExport void Native_FreeShaderCompiler()
{
    for (auto i = 0; i < _shaderCompilerProviders.size(); i++)
    {
        delete _shaderCompilerProviders[i];
    }
}

DllExport bool Native_CanCompileShader(ShaderLanguage shaderLanguage, ToolsGraphicsApi graphicsApi)
{
    if (!_platformTargetLanguages.count(graphicsApi))
    {
        return false;
    }

    auto graphicsApiTargetShaderLanguage = _platformTargetLanguages[graphicsApi];

    ShaderCompilerProvider* shaderCompilerProviders[10];
    auto result = FindShaderCompilersChain(shaderCompilerProviders, 0, shaderLanguage, graphicsApiTargetShaderLanguage);
    return result > 0;
}