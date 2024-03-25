#include "ElementalTools.h"
#include "DirectXShaderCompiler.h"
#include "SystemMemory.h"

typedef bool (*CheckCompilerPtr)();

struct ShaderCompiler
{
    ElemShaderLanguage InputLanguage;
    ReadOnlySpan<ElemShaderLanguage> OutputLanguages;
    CheckCompilerPtr CheckCompilerFunction; 
};

MemoryArena ShaderCompilerMemoryArena;
ReadOnlySpan<ShaderCompiler> shaderCompilers;

void InitShaderCompiler()
{
    if (!ShaderCompilerMemoryArena.Storage)
    {
        ShaderCompilerMemoryArena = SystemAllocateMemoryArena();

        ReadOnlySpan<ShaderCompiler> temp = 
        {
            {
                .InputLanguage = ElemShaderLanguage_Hlsl, 
                .OutputLanguages = { ElemShaderLanguage_Dxil },
                .CheckCompilerFunction = DirectXCompilerIsInstalled
            }
        };

        shaderCompilers = SystemDuplicateBuffer(ShaderCompilerMemoryArena, temp);
    }
}

ElemShaderLanguage GetApiTargetLanguage(ElemToolsGraphicsApi graphicsApi)
{
    switch (graphicsApi)
    {
        case ElemToolsGraphicsApi_DirectX12:
            return ElemShaderLanguage_Dxil;

        case ElemToolsGraphicsApi_Vulkan:
            return ElemShaderLanguage_Spirv;

        case ElemToolsGraphicsApi_Metal:
            return ElemShaderLanguage_MetalIR;
    }
}

ElemToolsAPI bool ElemCanCompileShader(ElemShaderLanguage shaderLanguage, ElemToolsGraphicsApi graphicsApi)
{
    InitShaderCompiler();

    auto targetLanguage = GetApiTargetLanguage(graphicsApi);

    for (uint32_t i = 0; i < shaderCompilers.Length; i++)
    {
        auto shaderCompiler = shaderCompilers[i];
        
        if (shaderCompiler.InputLanguage == shaderLanguage)
        {
            for (uint32_t j = 0; j < shaderCompiler.OutputLanguages.Length; j++)
            {
                if (shaderCompiler.OutputLanguages[j] == targetLanguage && shaderCompiler.CheckCompilerFunction())
                {
                    return true;
                }
            }
        }
    }

    return false;
}

