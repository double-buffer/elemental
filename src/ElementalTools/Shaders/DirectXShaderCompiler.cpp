#include "DirectXShaderCompiler.h"
#include "ShaderCompilerUtils.h"
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

bool ProcessDirectXShaderCompilerLogOutput(MemoryArena memoryArena, ComPtr<IDxcResult> compileResult, ElemToolsGraphicsApi targetGraphicsApi, Span<ElemToolsMessage> messages, uint32_t* messageIndex)
{
    auto hasErrors = false;

    ComPtr<IDxcBlobUtf8> pErrors;
    AssertIfFailed(compileResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&pErrors), nullptr));

    if (pErrors && pErrors->GetStringLength() > 0)
    {
        auto lines = SystemSplitString(memoryArena, (const char*)pErrors->GetBufferPointer(), '\n');

        for (size_t i = 0; i < lines.Length; i++)
        {
            auto currentLogType = ElemToolsMessageType_Information;
            auto line = SystemDuplicateBuffer(memoryArena, lines[i]);

            if (SystemFindSubString(line, "warning:") != -1)
            {
                currentLogType = ElemToolsMessageType_Warning;
            }
            else if (SystemFindSubString(line, "error:") != -1 || SystemFindSubString(line, "Unknown argument:") != -1)
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

ReadOnlySpan<char> GetShaderTypeTarget(DxilShaderKind shaderKind) 
{
    switch (shaderKind)
    {
        case DxilShaderKind::Compute:
            return "cs_6_8";

        case DxilShaderKind::Mesh:
            return "ms_6_8";

        case DxilShaderKind::Pixel:
            return "ps_6_8";

        default:
            return "";
    }
}

ShaderType GetShaderTypeEnum(DxilShaderKind shaderKind) 
{
    switch (shaderKind)
    {
        case DxilShaderKind::Compute:
            return ShaderType_Compute;

        case DxilShaderKind::Mesh:
            return ShaderType_Mesh;

        case DxilShaderKind::Pixel:
            return ShaderType_Pixel;

        default:
            return ShaderType_Unknown;
    }
}

ComPtr<IDxcResult> CompileDirectXShader(ReadOnlySpan<uint8_t> shaderCode, ReadOnlySpan<char> target, ElemToolsGraphicsApi targetApi, ReadOnlySpan<char> entryPoint, const ElemCompileShaderOptions* options)
{
    // TODO: Allow passing the pass to have a correct name in the messages

    auto stackMemoryArena = SystemGetStackMemoryArena();
    
    ComPtr<IDxcCompiler3> compiler;
    AssertIfFailed(directXShaderCompilerCreateInstanceFunction(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler)));

    auto parameters = SystemPushArray<const wchar_t*>(stackMemoryArena, 64);
    auto parameterIndex = 0u;

    parameters[parameterIndex++] = L"-Wno-ignored-attributes";

    parameters[parameterIndex++] = L"-T";
    parameters[parameterIndex++] = SystemConvertUtf8ToWideChar(stackMemoryArena, target).Pointer;

    if (entryPoint.Length > 0)
    {
        parameters[parameterIndex++] = L"-E";
        parameters[parameterIndex++] = SystemConvertUtf8ToWideChar(stackMemoryArena, entryPoint).Pointer;
    }

    if (targetApi == ElemToolsGraphicsApi_Vulkan)
    {
        parameters[parameterIndex++] = L"-spirv";
        parameters[parameterIndex++] = L"-fspv-target-env=vulkan1.3";
        parameters[parameterIndex++] = L"-fvk-use-dx-layout";
        // TODO: Add an options for this
        // TODO: To review
        parameters[parameterIndex++] = L"-fspv-use-legacy-buffer-matrix-order";

        parameters[parameterIndex++] = L"-fvk-bind-resource-heap";
        parameters[parameterIndex++] = L"0";
        parameters[parameterIndex++] = L"0";

        parameters[parameterIndex++] = L"-fvk-bind-sampler-heap";
        parameters[parameterIndex++] = L"0";
        parameters[parameterIndex++] = L"1";
    }

    parameters[parameterIndex++] = L"-HV";
    parameters[parameterIndex++] = L"202x";
    parameters[parameterIndex++] = L"-enable-16bit-types";

    parameters[parameterIndex++] = DXC_ARG_ALL_RESOURCES_BOUND;

    // TODO: Add an options for this
    parameters[parameterIndex++] = DXC_ARG_PACK_MATRIX_ROW_MAJOR;

    if (options && options->DebugMode)
    {
        parameters[parameterIndex++] = DXC_ARG_DEBUG;
        parameters[parameterIndex++] = L"-Qembed_debug";
    }

    DxcBuffer sourceBuffer;
    sourceBuffer.Ptr = shaderCode.Pointer;
    sourceBuffer.Size = shaderCode.Length;
    sourceBuffer.Encoding = 0;

    parameters = parameters.Slice(0, parameterIndex);

    ComPtr<IDxcResult> dxilCompileResult;
    AssertIfFailed(compiler->Compile(&sourceBuffer, parameters.Pointer, parameterIndex, nullptr, IID_PPV_ARGS(&dxilCompileResult)));

    return dxilCompileResult;
}

ComPtr<ID3D12ShaderReflection> GetDirectXShaderReflection(ComPtr<IDxcUtils> dxcUtils, ReadOnlySpan<uint8_t> shaderCode, ReadOnlySpan<char> target, ReadOnlySpan<char> entryPoint, const ElemCompileShaderOptions* options)
{
    ComPtr<IDxcResult> dxilCompileResult = CompileDirectXShader(shaderCode, target, ElemToolsGraphicsApi_DirectX12, entryPoint, options);

    ComPtr<IDxcBlob> shaderReflectionBlob;
    AssertIfFailed(dxilCompileResult->GetOutput(DXC_OUT_REFLECTION, IID_PPV_ARGS(&shaderReflectionBlob), nullptr));

    DxcBuffer reflectionBuffer =
    {
        .Ptr = shaderReflectionBlob->GetBufferPointer(),
        .Size = shaderReflectionBlob->GetBufferSize(),
        .Encoding = 0
    };

    ComPtr<ID3D12ShaderReflection> shaderReflection;
    AssertIfFailed(dxcUtils->CreateReflection(&reflectionBuffer, IID_PPV_ARGS(&shaderReflection)));

    return shaderReflection;
}

bool DirectXShaderCompilerIsInstalled()
{
    InitDirectXShaderCompiler();

    return directXShaderCompilerCreateInstanceFunction != nullptr;
}

ElemShaderCompilationResult DirectXShaderCompilerCompileShader(MemoryArena memoryArena, ReadOnlySpan<uint8_t> shaderCode, ElemShaderLanguage targetLanguage, ElemToolsGraphicsApi targetGraphicsApi, ElemToolsPlatform targetPlatform, const ElemCompileShaderOptions* options)
{
    auto stackMemoryArena = SystemGetStackMemoryArena();

    InitDirectXShaderCompiler();
    SystemAssert(directXShaderCompilerCreateInstanceFunction != nullptr);

    auto compilationMessages = SystemPushArray<ElemToolsMessage>(memoryArena, 1024);
    auto compilationMessageIndex = 0u;

    auto dxilCompileResult = CompileDirectXShader(shaderCode, "lib_6_8", ElemToolsGraphicsApi_DirectX12, "", options);
    auto hasErrors = ProcessDirectXShaderCompilerLogOutput(memoryArena, dxilCompileResult, targetGraphicsApi, compilationMessages, &compilationMessageIndex);

    auto outputShaderDataList = SystemPushArray<ShaderPart>(stackMemoryArena, 64);
    auto outputShaderDataListIndex = 0u;

    if (!hasErrors)
    {
        ComPtr<IDxcBlob> shaderByteCodeComPtr;
        AssertIfFailed(dxilCompileResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderByteCodeComPtr), nullptr));

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

            auto dxilShaderType = GetVersionShaderType(functionDescription.Version);
            auto shaderType = GetShaderTypeEnum(dxilShaderType);

            if (shaderType != ShaderType_Unknown)
            {
                auto dxilCompileResult = CompileDirectXShader(shaderCode, GetShaderTypeTarget(dxilShaderType), targetGraphicsApi, functionDescription.Name, options);
   
                auto hasErrors = ProcessDirectXShaderCompilerLogOutput(memoryArena, dxilCompileResult, targetGraphicsApi, compilationMessages, &compilationMessageIndex);

                if (hasErrors)
                {
                    continue;
                }

                ComPtr<IDxcBlob> shaderByteCodeComPtr;
                AssertIfFailed(dxilCompileResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderByteCodeComPtr), nullptr));

                auto shaderReflection = GetDirectXShaderReflection(dxcUtils, shaderCode, GetShaderTypeTarget(dxilShaderType), functionDescription.Name, options);

                uint32_t threadCountX, threadCountY, threadCountZ;
                shaderReflection->GetThreadGroupSize(&threadCountX, &threadCountY, &threadCountZ);

                auto metadata = SystemPushArray<ShaderMetadata>(stackMemoryArena, 1);
                auto currentMetadataIndex = 0u;

                metadata[currentMetadataIndex++] = { .Type = ShaderMetadataType_ThreadGroupSize, .Value = { threadCountX, threadCountY, threadCountZ } };

                auto shaderByteCode = Span<uint8_t>((uint8_t*)shaderByteCodeComPtr->GetBufferPointer(), shaderByteCodeComPtr->GetBufferSize());

                outputShaderDataList[outputShaderDataListIndex++] = 
                {
                    .ShaderType = shaderType,
                    .Name = SystemDuplicateBuffer(stackMemoryArena, ReadOnlySpan<char>(functionDescription.Name)),
                    .Metadata = metadata,
                    .ShaderCode = SystemDuplicateBuffer<uint8_t>(stackMemoryArena, shaderByteCode),
                };
            }
        }
    } 
    
    auto outputShaderData = Span<uint8_t>();

    if (!hasErrors)
    {
        outputShaderData = CombineShaderParts(memoryArena, outputShaderDataList.Slice(0, outputShaderDataListIndex)); 
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
