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

    if (CheckDxilShaderDataHeader(shaderCode, "DXBC"))
    {
        printf("DXIL LIB\n");
    }  
    else
    {
        auto dataSpan = Span<uint8_t>((uint8_t*)shaderCode.Pointer, shaderCode.Length);
        auto shaderCount = *(uint32_t*)dataSpan.Pointer;
        dataSpan = dataSpan.Slice(sizeof(uint32_t));
        printf("Graphics LIB %d\n", shaderCount);

        auto graphicsShaderData = SystemPushArray<ReadOnlySpan<uint8_t>>(stackMemoryArena, shaderCount);

        for (uint32_t i = 0; i < shaderCount; i++)
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
                printf("Errror metal :(\n");
                // Inspect pError to determine cause.
                IRErrorDestroy( pError );
            }

            // TODO: We need to have shader parts
            auto result = IRObjectGetMetalLibBinary(pOutIR, IRShaderStageAmplification, pMetallib);

            if (!result)
            {
                result = IRObjectGetMetalLibBinary(pOutIR, IRShaderStageMesh, pMetallib);
            }

            if (!result)
            {
                result = IRObjectGetMetalLibBinary(pOutIR, IRShaderStageFragment, pMetallib);
            }

            printf("Result: %d\n", result);

            size_t metallibSize = IRMetalLibGetBytecodeSize(pMetallib);
            printf("Bytecodesize = %lu\n", metallibSize);
            uint8_t* metallib = new uint8_t[metallibSize];
            IRMetalLibGetBytecode(pMetallib, metallib);
        }
    }  

    return {};
}
