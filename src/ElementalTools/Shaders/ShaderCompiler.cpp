#include "ElementalTools.h"
#include "ToolsUtils.h"
#include "DirectXShaderCompiler.h"
#include "MetalShaderConverter.h"
#include "SystemFunctions.h"
#include "SystemMemory.h"

#define SHADERCOMPILER_MAX_COMPILERS 32 

typedef bool (*CheckCompilerPtr)();
typedef ElemShaderCompilationResult (*CompileShaderPtr)(MemoryArena memoryArena, ReadOnlySpan<uint8_t> shaderCode, ElemShaderLanguage targetLanguage, ElemToolsGraphicsApi targetGraphicsApi, ElemToolsPlatform targetPlatform, const ElemCompileShaderOptions* options);

struct ShaderCompiler
{
    ElemShaderLanguage InputLanguage;
    ReadOnlySpan<ElemShaderLanguage> OutputLanguages;
    CheckCompilerPtr CheckCompilerFunction; 
    CompileShaderPtr CompileShaderFunction;
};

struct ShaderCompilerStep
{
    const ShaderCompiler* ShaderCompiler;
    ElemShaderLanguage InputLanguage;
    ElemShaderLanguage OutputLanguage;
};

MemoryArena ShaderCompilerMemoryArena;
ReadOnlySpan<ShaderCompiler> shaderCompilers;

ReadOnlySpan<ElemShaderLanguage> InitShaderLanguages(std::initializer_list<ElemShaderLanguage> initList)
{
    auto array = SystemPushArray<ElemShaderLanguage>(ShaderCompilerMemoryArena, initList.size());

    for (uint32_t i = 0; i < initList.size(); i++)
    {
        array[i] = initList.begin()[i];
    }

    return ReadOnlySpan<ElemShaderLanguage>(array);
}

void InitShaderCompiler()
{
    if (!ShaderCompilerMemoryArena.Storage)
    {
        ShaderCompilerMemoryArena = SystemAllocateMemoryArena();

        auto compilerArray = SystemPushArray<ShaderCompiler>(ShaderCompilerMemoryArena, SHADERCOMPILER_MAX_COMPILERS);
        uint32_t shaderCompilerIndex = 0;

        compilerArray[shaderCompilerIndex++] = {
            .InputLanguage = ElemShaderLanguage_Hlsl, 
            .OutputLanguages = InitShaderLanguages({ ElemShaderLanguage_Dxil, ElemShaderLanguage_Spirv }),
            .CheckCompilerFunction = DirectXShaderCompilerIsInstalled,
            .CompileShaderFunction = DirectXShaderCompilerCompileShader
        };

        #ifndef __linux__
        compilerArray[shaderCompilerIndex++] = {
            .InputLanguage = ElemShaderLanguage_Dxil, 
            .OutputLanguages = InitShaderLanguages({ ElemShaderLanguage_MetalIR }),
            .CheckCompilerFunction = MetalShaderConverterIsInstalled,
            .CompileShaderFunction = MetalShaderConverterCompileShader
        };
        #endif

        shaderCompilers = compilerArray.Slice(0, shaderCompilerIndex);
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

ElemShaderLanguage GetShaderLanguageFromPath(const char* path)
{
    auto pathSpan = ReadOnlySpan<char>(path);
    auto stackMemoryArena = SystemGetStackMemoryArena();

    auto lastIndex = SystemLastIndexOf(path, '.');
    SystemAssert(lastIndex != -1);

    auto extension = SystemPushArray<char>(stackMemoryArena, pathSpan.Length - lastIndex);
    SystemCopyBuffer(extension, pathSpan.Slice(lastIndex + 1));

    if (SystemFindSubString(extension, "hlsl") != -1)
    {
        return ElemShaderLanguage_Hlsl;
    }

    return ElemShaderLanguage_Unknown;
}

void ReverseCompilerChainArray(Span<ShaderCompilerStep> steps, uint32_t count)
{
    for (uint32_t i = 0; i < count - 1; i++)
    {
        auto temp = steps[count - i - 1];
        steps[count - i - 1] = steps[i];
        steps[i] = temp;
    }
}

ReadOnlySpan<const ShaderCompiler*> FindShaderCompilersForTargetLanguage(ElemShaderLanguage targetLanguage)
{
    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto result = SystemPushArray<const ShaderCompiler*>(stackMemoryArena, SHADERCOMPILER_MAX_COMPILERS);
    auto currentIndex = 0;
    
    for (uint32_t i = 0; i < shaderCompilers.Length; i++)
    {
        auto shaderCompiler = &shaderCompilers[i];

        for (uint32_t j = 0; j < shaderCompiler->OutputLanguages.Length; j++)
        {
            if (shaderCompiler->OutputLanguages[j] == targetLanguage && shaderCompiler->CheckCompilerFunction())
            {
                result[currentIndex++] = shaderCompiler;
            }
        }
    }

    return result.Slice(0, currentIndex);
}

bool FindShaderCompilerChain(ElemShaderLanguage sourceLanguage, ElemShaderLanguage targetLanguage, Span<ShaderCompilerStep> compilerSteps, uint32_t* level)
{
    *level = *level + 1;
    auto localLevel = *level;
    auto shaderCompilers = FindShaderCompilersForTargetLanguage(targetLanguage);

    for (uint32_t i = 0; i < shaderCompilers.Length; i++)
    {
        auto shaderCompiler = shaderCompilers[i];

        compilerSteps[0] = 
        {
            .ShaderCompiler = shaderCompiler,
            .InputLanguage = shaderCompiler->InputLanguage,
            .OutputLanguage = targetLanguage
        };
            
        if (shaderCompiler->InputLanguage == sourceLanguage || FindShaderCompilerChain(sourceLanguage, shaderCompiler->InputLanguage, compilerSteps.Slice(*level), level))
        {
            if (localLevel == 1)
            {
                ReverseCompilerChainArray(compilerSteps, *level);

                if (compilerSteps[0].InputLanguage != sourceLanguage)
                {
                    return false;
                }
            }

            return true;
        }
    }

    level = 0;
    return false;
}

ElemToolsAPI bool ElemCanCompileShader(ElemShaderLanguage shaderLanguage, ElemToolsGraphicsApi graphicsApi, ElemToolsPlatform platform)
{
    InitShaderCompiler();
    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto compilerSteps = SystemPushArray<ShaderCompilerStep>(stackMemoryArena, SHADERCOMPILER_MAX_COMPILERS);
    auto targetLanguage = GetApiTargetLanguage(graphicsApi);
    auto level = 0u;

    return FindShaderCompilerChain(shaderLanguage, targetLanguage, compilerSteps, &level);
}

ElemToolsAPI ElemShaderCompilationResult ElemCompileShaderLibrary(ElemToolsGraphicsApi graphicsApi, ElemToolsPlatform platform, const char* path, const ElemCompileShaderOptions* options)
{
    SystemAssert(path);

    InitShaderCompiler();
    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto compilerSteps = SystemPushArray<ShaderCompilerStep>(stackMemoryArena, SHADERCOMPILER_MAX_COMPILERS);
    auto targetLanguage = GetApiTargetLanguage(graphicsApi);
    auto level = 0u;

    auto compilationMessages = Span<ElemToolsMessage>();
    auto compilationData = ReadOnlySpan<uint8_t>();

    auto shaderLanguage = GetShaderLanguageFromPath(path);
    SystemAssert(shaderLanguage != ElemShaderLanguage_Unknown);

    if (!FindShaderCompilerChain(shaderLanguage, targetLanguage, compilerSteps, &level))
    {
        return
        {
            .Messages = ConstructErrorMessageSpan(ShaderCompilerMemoryArena, "Cannot find a compatible shader compilers chain."),
            .HasErrors = true
        };
    }

    auto hasErrors = false;

    auto stepSourceData = LoadFileData(path); 

    if (stepSourceData.Length == 0)
    {
        return
        {
            .Messages = ConstructErrorMessageSpan(ShaderCompilerMemoryArena, "Cannot read input file."),
            .HasErrors = true
        };
    }

    for (uint32_t i = 0; i < level; i++)
    {
        auto compilerStep = compilerSteps[i];
        auto compilationResults = compilerStep.ShaderCompiler->CompileShaderFunction(stackMemoryArena, stepSourceData, compilerStep.OutputLanguage, graphicsApi, platform, options);

        auto resultMessages = SystemPushArray<ElemToolsMessage>(stackMemoryArena, compilationResults.Messages.Length);

        for (uint32_t j = 0; j < compilationResults.Messages.Length; j++)
        {
            auto compilationMessage = compilationResults.Messages.Items[j];
            resultMessages[j].Type = compilationMessage.Type;
            resultMessages[j].Message = SystemDuplicateBuffer<char>(stackMemoryArena, compilationMessage.Message).Pointer;

            if (compilationMessage.Type == ElemToolsMessageType_Error)
            {
                hasErrors = true;
            }
        }

        compilationMessages = SystemConcatBuffers(stackMemoryArena, ReadOnlySpan<ElemToolsMessage>(compilationMessages), ReadOnlySpan<ElemToolsMessage>(resultMessages)); 

        if (hasErrors)
        {
            break;
        }

        compilationData = SystemDuplicateBuffer<uint8_t>(stackMemoryArena, ReadOnlySpan<uint8_t>(compilationResults.Data.Items, compilationResults.Data.Length));
        stepSourceData = compilationData;
    }

    ResetLoadFileDataMemory();

    return 
    {
        .Data = 
        {
            .Items = (uint8_t*)compilationData.Pointer,
            .Length = (uint32_t)compilationData.Length
        },
        .Messages = 
        {
            .Items = compilationMessages.Pointer,
            .Length = (uint32_t)compilationMessages.Length
        },
        .HasErrors = hasErrors
    };
}
