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

ElemShaderCompilationResult MetalShaderConverterCompileShader(MemoryArena memoryArena, ReadOnlySpan<uint8_t> shaderCode, ElemShaderLanguage targetLanguage, ElemToolsGraphicsApi targetGraphicsApi, const ElemCompileShaderOptions* options)
{
    auto stackMemoryArena = SystemGetStackMemoryArena();
    IRCompiler* pCompiler = IRCompilerCreate();
    IRMetalLibBinary* pMetallib = IRMetalLibBinaryCreate();
    
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
            .Num32BitValues = 16
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
        auto dataSpan = Span<uint8_t>((uint8_t*)shaderCode.Pointer, shaderCode.Length);
        auto shaderCount = *(uint32_t*)dataSpan.Pointer;
        dataSpan = dataSpan.Slice(sizeof(uint32_t));
        printf("Graphics LIB %d\n", shaderCount);

        // HACK: For dx the first one is the lib, we need to refactor this
        auto outputShaderDataList = SystemPushArray<ShaderPart>(stackMemoryArena, shaderCount + 1);

        for (uint32_t i = 1; i < shaderCount + 1; i++)
        {
            auto size = *(uint32_t*)dataSpan.Pointer;
            dataSpan = dataSpan.Slice(sizeof(uint32_t));

            auto dxilShaderData = ReadOnlySpan<uint8_t>(dataSpan.Pointer, size);
            dataSpan = dataSpan.Slice(size);

            IRObject* pDXIL = IRObjectCreateFromDXIL(dxilShaderData.Pointer, dxilShaderData.Length, IRBytecodeOwnershipNone);

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
            auto result = IRObjectGetMetalLibBinary(pOutIR, IRShaderStageAmplification, pMetallib);
            auto shaderType = ShaderType_Amplification;

            if (!result)
            {
                result = IRObjectGetMetalLibBinary(pOutIR, IRShaderStageMesh, pMetallib);
                shaderType = ShaderType_Mesh;
            }

            if (!result)
            {
                result = IRObjectGetMetalLibBinary(pOutIR, IRShaderStageFragment, pMetallib);
                shaderType = ShaderType_Pixel;
            }

            printf("Result: %d\n", result);

            size_t metallibSize = IRMetalLibGetBytecodeSize(pMetallib);
            printf("Bytecodesize = %lu\n", metallibSize);

            auto shaderByteCode = SystemPushArray<uint8_t>(stackMemoryArena, metallibSize); 
            IRMetalLibGetBytecode(pMetallib, shaderByteCode.Pointer);

            outputShaderDataList[i] = 
            {
                .ShaderType = shaderType, 
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
