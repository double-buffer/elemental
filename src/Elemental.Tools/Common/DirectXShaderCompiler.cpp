#include "DirectXShaderCompiler.h"
#include "SystemFunctions.h"
#include "SystemMemory.h"

SystemLibrary directXShaderCompilerLibrary = {};
DxcCreateInstanceProc directXShaderCompilerCreateInstanceFunction = nullptr;

void InitDirectXShaderCompiler()
{
    if (!directXShaderCompilerLibrary.Handle)
    {
        directXShaderCompilerLibrary = SystemLoadLibrary("dxcompiler");

        if (directXShaderCompilerLibrary.Handle != nullptr)
        {
            directXShaderCompilerCreateInstanceFunction = (DxcCreateInstanceProc)SystemGetFunctionExport(directXShaderCompilerLibrary, "DxcCreateInstance");
        }
    }
}

bool ProcessDirectXShaderCompilerLogOutput(MemoryArena memoryArena, ComPtr<IDxcResult> compileResult, Span<ElemToolsMessage> messages, uint32_t* messageIndex)
{
    auto hasErrors = false;

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
    }

    return hasErrors;
}

DxilShaderKind GetVersionShaderType(uint32_t programVersion) 
{
    return (DxilShaderKind)((programVersion & 0xffff0000) >> 16);
}

bool DirectXShaderCompilerIsInstalled()
{
    InitDirectXShaderCompiler();

    return directXShaderCompilerCreateInstanceFunction != nullptr;
}

ElemShaderCompilationResult DirectXShaderCompilerCompileShader(ReadOnlySpan<uint8_t> shaderCode, ElemShaderLanguage targetLanguage, ElemToolsGraphicsApi targetGraphicsApi, const ElemCompileShaderOptions* options)
{
    InitDirectXShaderCompiler();
    SystemAssert(directXShaderCompilerCreateInstanceFunction != nullptr);

    auto stackMemoryArena = SystemGetStackMemoryArena();

    auto compilationMessages = SystemPushArray<ElemToolsMessage>(stackMemoryArena, 1024);
    auto compilationMessageIndex = 0u;
    
    ComPtr<IDxcCompiler3> compiler;
    AssertIfFailed(directXShaderCompilerCreateInstanceFunction(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler)));

    auto parameters = SystemPushArray<const wchar_t*>(stackMemoryArena, 64);
    auto parameterIndex = 0u;

    parameters[parameterIndex++] = L"-T";
    parameters[parameterIndex++] = L"lib_6_8";

    /*
    // TODO: Use utils function to build parameters
    std::vector<const wchar_t*> arguments;
    arguments.push_back(L"-HV 2021");

    //-E for the entry point (eg. PSMain)
    arguments.push_back(L"-E");
    
    auto entryPointString = SystemConvertUtf8ToWideChar(stackMemoryArena, (char*)entryPoint);
    arguments.push_back(entryPointString.Pointer);

    // TODO: Use defines
    auto shaderTarget = L"ms_6_7";

    if (shaderStage == ShaderStage_AmplificationShader)
    {
        shaderTarget = L"as_6_7";
    }
    else if (shaderStage == ShaderStage_PixelShader)
    {
        shaderTarget = L"ps_6_7";
    }

    arguments.push_back(L"-T");
    arguments.push_back(shaderTarget);
    

    // TODO: pass all-resources-bound!!!
    
            
    // TODO: Additional options to be considered
    // Pass a parameter for debug and release mode
*/
    /*
    //Strip reflection data and pdbs (see later)
    arguments.push_back(L"-Qstrip_debug");
    arguments.push_back(L"-Qstrip_reflect");

    arguments.push_back(DXC_ARG_WARNINGS_ARE_ERRORS); //-WX
    arguments.push_back(DXC_ARG_DEBUG); //-Zi
    arguments.push_back(DXC_ARG_PACK_MATRIX_ROW_MAJOR); //-Zp


    for (const std::wstring& define : defines)
    {
        arguments.push_back(L"-D");
        arguments.push_back(define.c_str());
    }*/

    DxcBuffer sourceBuffer;
    sourceBuffer.Ptr = shaderCode.Pointer;
    sourceBuffer.Size = shaderCode.Length;
    sourceBuffer.Encoding = 0;

    parameters = parameters.Slice(0, parameterIndex);

    ComPtr<IDxcResult> dxilCompileResult;
    AssertIfFailed(compiler->Compile(&sourceBuffer, parameters.Pointer, parameterIndex, nullptr, IID_PPV_ARGS(&dxilCompileResult)));

    auto hasErrors = ProcessDirectXShaderCompilerLogOutput(stackMemoryArena, dxilCompileResult, compilationMessages, &compilationMessageIndex);
    Span<uint8_t> outputShaderData; 

    if (!hasErrors)
    {
        ComPtr<IDxcBlob> shaderByteCodeComPtr;
        AssertIfFailed(dxilCompileResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderByteCodeComPtr), nullptr));

        auto shaderByteCode = Span<uint8_t>((uint8_t*)shaderByteCodeComPtr->GetBufferPointer(), shaderByteCodeComPtr->GetBufferSize());
        outputShaderData = SystemDuplicateBuffer<uint8_t>(stackMemoryArena, shaderByteCode);

        // Reflection
        ComPtr<IDxcUtils> dxcUtils;
        AssertIfFailed(directXShaderCompilerCreateInstanceFunction(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils)));
        
        ComPtr<IDxcBlob> shaderReflectionBlob;
        AssertIfFailed(dxilCompileResult->GetOutput(DXC_OUT_REFLECTION, IID_PPV_ARGS(&shaderReflectionBlob), nullptr));

        DxcBuffer reflectionBuffer;
        reflectionBuffer.Ptr = shaderReflectionBlob->GetBufferPointer();
        reflectionBuffer.Size = shaderReflectionBlob->GetBufferSize();
        reflectionBuffer.Encoding = 0;

        ComPtr<ID3D12LibraryReflection> shaderReflection;
        AssertIfFailed(dxcUtils->CreateReflection(&reflectionBuffer, IID_PPV_ARGS(&shaderReflection)));

        D3D12_LIBRARY_DESC libraryDescription;
        AssertIfFailed(shaderReflection->GetDesc(&libraryDescription));

        for (uint32_t i = 0; i < libraryDescription.FunctionCount; i++)
        {
            auto functionReflection = shaderReflection->GetFunctionByIndex(i);

            D3D12_FUNCTION_DESC functionDescription;
            functionReflection->GetDesc(&functionDescription);

            printf("Function %s (%u)\n", functionDescription.Name, GetVersionShaderType(functionDescription.Version));
        }
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
    };
}
