#include "MetalShaderConverter.h"
#include "ShaderCompilerUtils.h"
#include "SystemFunctions.h"
#include "SystemMemory.h"

bool ProcessMetalShaderConverterLogOutput(MemoryArena memoryArena, ComPtr<IDxcResult> compileResult, Span<ElemToolsMessage> messages, uint32_t* messageIndex)
{
    auto hasErrors = false;
/*
    // TODO: We cannot use the normal string find methods here because we work directly in UTF8

    ComPtr<IDxcBlobUtf8> pErrors;
    AssertIfFailed(compileResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&pErrors), nullptr));

    if (pErrors && pErrors->GetStringLength() > 0)
    {
        auto lines = SystemSplitString(memoryArena, (const char*)pErrors->GetBufferPointer(), '\n');

        for (size_t i = 0; i < lines.Length; i++)
        {
            auto currentLogType = ElemToolsMessageType_Information;
            auto line = SystemDuplicateBuffer(memoryArena, lines[i]);

            if (line.Length == 0)
            {
                continue;
            }

            if (SystemFindSubString(line, "warning:") != -1)
            {
                currentLogType = ElemToolsMessageType_Warning;
            }
            else if (SystemFindSubString(line, "error:") != -1)
            {
                currentLogType = ElemToolsMessageType_Error;
                hasErrors = true;
            }

            messages[*messageIndex] =
            {
                .Type = currentLogType,
                .Message = line.Pointer
            };

            *messageIndex += 1;
        }
}*/

    return hasErrors;
}

IRShaderStage ConvertShaderTypeToMetalShaderStage(ShaderType shaderType)
{
    switch (shaderType)
    {
        case ShaderType_Amplification:
            return IRShaderStage::IRShaderStageAmplification;

        case ShaderType_Mesh:
            return IRShaderStage::IRShaderStageMesh;

        case ShaderType_Pixel:
            return IRShaderStage::IRShaderStageFragment;

        case ShaderType_Compute:
        case ShaderType_Library:
        case ShaderType_Unknown:
            return IRShaderStage::IRShaderStageCompute;
    };
}

bool CheckDxilShaderDataHeader(ReadOnlySpan<uint8_t> data, const char* headerValue)
{
    for (uint32_t i = 0; i < 4; i++)
    {
        if (data[i] != headerValue[i])
        {
            return false;
        }
    }

    return true;
}

bool MetalShaderConverterIsInstalled()
{
    IRCompiler* compiler = IRCompilerCreate();
    return compiler != nullptr;
}

ElemShaderCompilationResult MetalShaderConverterCompileShader(MemoryArena memoryArena, ReadOnlySpan<uint8_t> shaderCode, ElemShaderLanguage targetLanguage, ElemToolsGraphicsApi targetGraphicsApi, ElemToolsPlatform targetPlatform, const ElemCompileShaderOptions* options)
{
    auto stackMemoryArena = SystemGetStackMemoryArena();
    IRCompiler* pCompiler = IRCompilerCreate();
    IRMetalLibBinary* pMetallib = IRMetalLibBinaryCreate();

    IRCompilerSetMinimumGPUFamily(pCompiler, IRGPUFamilyMetal3);

    if (targetPlatform == ElemToolsPlatform_iOS)
    {
        IRCompilerSetMinimumDeploymentTarget(pCompiler, IROperatingSystem_iOS, "17.0.0");
    }
    else
    {
        IRCompilerSetMinimumDeploymentTarget(pCompiler, IROperatingSystem_macOS, "15.0.0");
    }    

    auto compilationMessages = SystemPushArray<ElemToolsMessage>(memoryArena, 1024);
    auto compilationMessageIndex = 0u;
    auto hasErrors = false;

    IRRootParameter1 rootParameter = 
    {
        .ParameterType = IRRootParameterType32BitConstants,
        .Constants = 
        {
            .ShaderRegister = 0,
            .RegisterSpace = 0,
            .Num32BitValues = 24
        },
        .ShaderVisibility = IRShaderVisibilityAll
    };

    IRVersionedRootSignatureDescriptor rootSignatureDescriptor
    {
        .version = IRRootSignatureVersion_1_1,
        .desc_1_1 =
        {
            .NumParameters = 1,
            .pParameters = &rootParameter,
            .Flags = IRRootSignatureFlags(IRRootSignatureFlagCBVSRVUAVHeapDirectlyIndexed)
        }
    };
    IRError* rootSignatureError = nullptr;
    auto rootSignature = IRRootSignatureCreateFromDescriptor(&rootSignatureDescriptor, &rootSignatureError);

    if (rootSignatureError)
    {
        char* error_msg = (char*)IRErrorGetPayload(rootSignatureError);
        printf("Error Root Signature %s\n", error_msg);
        // Inspect pError to determine cause.
        IRErrorDestroy( rootSignatureError );
    }

    IRCompilerSetGlobalRootSignature(pCompiler, rootSignature);

    if (CheckDxilShaderDataHeader(shaderCode, "DXBC"))
    {
        printf("DXIL LIB\n");
        return {};
    }  
    else
    {
        auto shaderParts = ReadShaderParts(stackMemoryArena, shaderCode);
        auto outputShaderDataList = SystemPushArray<ShaderPart>(stackMemoryArena, shaderParts.Length);

        for (uint32_t i = 0; i < shaderParts.Length; i++)
        {
            auto shaderPart = shaderParts[i];

            IRObject* pDXIL = IRObjectCreateFromDXIL(shaderPart.ShaderCode.Pointer, shaderPart.ShaderCode.Length, IRBytecodeOwnershipNone);

            // Compile DXIL to Metal IR:
            IRError* pError = nullptr;
            IRObject* pOutIR = IRCompilerAllocCompileAndLink(pCompiler, NULL,  pDXIL, &pError);

            if (!pOutIR)
            {
                hasErrors = true;
                char* error_msg = (char*)IRErrorGetPayload(pError);
                auto line = SystemDuplicateBuffer(memoryArena, ReadOnlySpan<char>(error_msg));

                compilationMessages[compilationMessageIndex++] =
                {
                    .Type = ElemToolsMessageType_Error,
                    .Message = line.Pointer
                };

                // Inspect pError to determine cause.
                IRErrorDestroy( pError );

                break;
            }

            // TODO: We need to have shader parts
            auto result = IRObjectGetMetalLibBinary(pOutIR, ConvertShaderTypeToMetalShaderStage(shaderPart.ShaderType), pMetallib);

            if (!result)
            {
                // TODO: Error
                printf("ERRORRRRRR!\n");
            }

            size_t metallibSize = IRMetalLibGetBytecodeSize(pMetallib);
            auto shaderByteCode = SystemPushArray<uint8_t>(stackMemoryArena, metallibSize); 
            IRMetalLibGetBytecode(pMetallib, shaderByteCode.Pointer);

            outputShaderDataList[i] = 
            {
                .ShaderType = shaderPart.ShaderType, 
                .Name = SystemDuplicateBuffer(stackMemoryArena, shaderPart.Name), 
                .Metadata = shaderPart.Metadata,
                .ShaderCode = SystemDuplicateBuffer<uint8_t>(stackMemoryArena, shaderByteCode)
            };
        }

        auto outputShaderData = CombineShaderParts(memoryArena, outputShaderDataList); 

        return 
        {
            .Data = 
            {
                .Items = outputShaderData.Pointer,
                .Length = (uint32_t)outputShaderData.Length
            },
            .Messages = 
            {
                .Items = compilationMessages.Slice(0, compilationMessageIndex).Pointer,
                .Length = compilationMessageIndex,
            },
            .HasErrors = hasErrors
        };
    }  
}
