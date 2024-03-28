#include "MetalShaderConverter.h"
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

bool MetalShaderConverterIsInstalled()
{
    IRCompiler* compiler = IRCompilerCreate();
    return compiler != nullptr;
}

ElemShaderCompilationResult MetalShaderConverterCompileShader(ReadOnlySpan<uint8_t> shaderCode, ElemShaderLanguage targetLanguage, ElemToolsGraphicsApi targetGraphicsApi, const ElemCompileShaderOptions* options)
{
    uint32_t sizeFirstBlock = *(uint32_t*)shaderCode.Pointer;
    auto firstBlock = shaderCode.Slice(sizeof(uint32_t), sizeFirstBlock);
    auto secondBlock = shaderCode.Slice(sizeof(uint32_t) + sizeFirstBlock, shaderCode.Length - (sizeof(uint32_t) + sizeFirstBlock));

    IRCompiler* pCompiler = IRCompilerCreate();

    IRObject* pDXIL = IRObjectCreateFromDXIL(firstBlock.Pointer, firstBlock.Length, IRBytecodeOwnershipNone);

    // Compile DXIL to Metal IR:
    IRError* pError = nullptr;
    IRObject* pOutIR = IRCompilerAllocCompileAndLink(pCompiler, NULL,  pDXIL, &pError);

    if (!pOutIR)
    {
            printf("Errror metal :(\n");
      // Inspect pError to determine cause.
      IRErrorDestroy( pError );
    }

    if (secondBlock.Length > 0)
    {
        // TODO: Is it needed?
        IRObject* pDXIL2 = IRObjectCreateFromDXIL(secondBlock.Pointer, secondBlock.Length, IRBytecodeOwnershipNone);

        // Compile DXIL to Metal IR:
        IRError* pError2 = nullptr;
        IRObject* pOutIR2 = IRCompilerAllocCompileAndLink(pCompiler, NULL,  pDXIL2, &pError2);

        if (!pOutIR2)
        {
                printf("Errror metal2 :(\n");
          // Inspect pError to determine cause.
          IRErrorDestroy( pError2 );
        }
    
        auto test = IRObjectGetMetalIRShaderStage(pOutIR2);
    }

    IRMetalLibBinary* pMetallib = IRMetalLibBinaryCreate();

    // If we pass an invalid stage, it seems it process the whole DXIL lib
    IRObjectGetMetalLibBinary(pOutIR, IRShaderStageInvalid, pMetallib);
    size_t metallibSize = IRMetalLibGetBytecodeSize(pMetallib);
    uint8_t* metallib = new uint8_t[metallibSize];
    IRMetalLibGetBytecode(pMetallib, metallib);

    /*SystemAssert(metalShaderConverterCreateInstanceFunction != nullptr);

    auto stackMemoryArena = SystemGetStackMemoryArena();

    auto compilationMessages = SystemPushArray<ElemToolsMessage>(stackMemoryArena, 1024);
    auto compilationMessageIndex = 0u;
    
    ComPtr<IDxcCompiler3> compiler;
    AssertIfFailed(metalShaderConverterCreateInstanceFunction(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler)));

    auto parameters = SystemPushArray<const wchar_t*>(stackMemoryArena, 64);
    auto parameterIndex = 0u;

    parameters[parameterIndex++] = L"-T";
    parameters[parameterIndex++] = L"lib_6_8";

    DxcBuffer sourceBuffer;
    sourceBuffer.Ptr = shaderCode.Pointer;
    sourceBuffer.Size = shaderCode.Length;
    sourceBuffer.Encoding = 0;

    parameters = parameters.Slice(0, parameterIndex);

    ComPtr<IDxcResult> dxilCompileResult;
    AssertIfFailed(compiler->Compile(&sourceBuffer, parameters.Pointer, (uint32_t)parameters.Length, nullptr, IID_PPV_ARGS(&dxilCompileResult)));

    auto hasErrors = ProcessMetalShaderConverterLogOutput(stackMemoryArena, dxilCompileResult, compilationMessages, &compilationMessageIndex);
    Span<uint8_t> outputShaderData; 

    if (!hasErrors)
    {
        ComPtr<IDxcBlob> shaderByteCodeComPtr;
        AssertIfFailed(dxilCompileResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderByteCodeComPtr), nullptr));

        auto shaderByteCode = Span<uint8_t>((uint8_t*)shaderByteCodeComPtr->GetBufferPointer(), shaderByteCodeComPtr->GetBufferSize());
        outputShaderData = SystemDuplicateBuffer<uint8_t>(stackMemoryArena, shaderByteCode);
    } 

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
    };*/

    return {};
}
