#include "ElementalTools.h"
#include "DirectXShaderCompiler.h"
#include "MetalShaderConverter.h"
#include "SystemFunctions.h"
#include "SystemMemory.h"

// TODO: Put magic numbers into define

typedef bool (*CheckCompilerPtr)();
typedef ElemShaderCompilationResult (*CompileShaderPtr)(ReadOnlySpan<uint8_t> shaderCode, ElemShaderLanguage targetLanguage, ElemToolsGraphicsApi targetGraphicsApi, const ElemCompileShaderOptions* options);

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

        auto compilerArray = SystemPushArray<ShaderCompiler>(ShaderCompilerMemoryArena, 32);
        uint32_t shaderCompilerIndex = 0;

        compilerArray[shaderCompilerIndex++] = {
            .InputLanguage = ElemShaderLanguage_Hlsl, 
            .OutputLanguages = InitShaderLanguages({ ElemShaderLanguage_Dxil, ElemShaderLanguage_Spirv }),
            .CheckCompilerFunction = DirectXShaderCompilerIsInstalled,
            .CompileShaderFunction = DirectXShaderCompilerCompileShader
        };

        compilerArray[shaderCompilerIndex++] = {
            .InputLanguage = ElemShaderLanguage_Dxil, 
            .OutputLanguages = InitShaderLanguages({ ElemShaderLanguage_MetalIR }),
            .CheckCompilerFunction = MetalShaderConverterIsInstalled,
            .CompileShaderFunction = MetalShaderConverterCompileShader
        };

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
    auto result = SystemPushArray<const ShaderCompiler*>(stackMemoryArena, 32);
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

ElemToolsAPI bool ElemCanCompileShader(ElemShaderLanguage shaderLanguage, ElemToolsGraphicsApi graphicsApi)
{
    InitShaderCompiler();
    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto compilerSteps = SystemPushArray<ShaderCompilerStep>(stackMemoryArena, 32);
    auto targetLanguage = GetApiTargetLanguage(graphicsApi);
    auto level = 0u;

    return FindShaderCompilerChain(shaderLanguage, targetLanguage, compilerSteps, &level);
}

ElemToolsAPI ElemShaderCompilationResult ElemCompileShaderLibrary(ElemToolsGraphicsApi graphicsApi, const ElemShaderSourceData* sourceData, const ElemCompileShaderOptions* options)
{
    SystemAssert(sourceData);

    InitShaderCompiler();
    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto compilerSteps = SystemPushArray<ShaderCompilerStep>(stackMemoryArena, 32);
    auto targetLanguage = GetApiTargetLanguage(graphicsApi);
    auto level = 0u;

    auto compilationMessages = Span<ElemToolsMessage>();
    auto compilationData = ReadOnlySpan<uint8_t>();

    if (!FindShaderCompilerChain(sourceData->ShaderLanguage, targetLanguage, compilerSteps, &level))
    {
        auto messageItem = SystemPushStruct<ElemToolsMessage>(stackMemoryArena);
        messageItem->Type = ElemToolsMessageType_Error;
        messageItem->Message = "Cannot find a compatible shader compilers chain.";

        return
        {
            .Messages = 
            {
                .Items = messageItem,
                .Length = 1
            },
            .HasErrors = true
        };
    }

    auto hasErrors = false;

    auto stepSourceData = ReadOnlySpan<uint8_t>((uint8_t*)sourceData->Data.Items, sourceData->Data.Length);

    for (uint32_t i = 0; i < level; i++)
    {
        auto compilerStep = compilerSteps[i];

        auto compilationResults = compilerStep.ShaderCompiler->CompileShaderFunction(stepSourceData, compilerStep.OutputLanguage, graphicsApi, options);

        auto resultMessages = SystemPushArray<ElemToolsMessage>(stackMemoryArena, compilationResults.Messages.Length);

        for (uint32_t j = 0; j < compilationResults.Messages.Length; j++)
        {
            auto compilationMessage = compilationResults.Messages.Items[j];
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
