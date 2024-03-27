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

ElemShaderCompilationResult MetalShaderConverterCompileShader(ReadOnlySpan<uint8_t> shaderCode, ElemShaderLanguage targetLanguage, const ElemCompileShaderOptions* options)
{
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
