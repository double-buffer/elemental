#include "DirectXShaderCompilerProvider.h"

DirectXShaderCompilerProvider::DirectXShaderCompilerProvider()
{
    _dxcompilerDll = SystemLoadLibrary("dxcompiler");

    if (_dxcompilerDll.Handle != nullptr)
    {
        _createInstanceFunc = (DxcCreateInstanceProc)SystemGetFunctionExport(_dxcompilerDll, "DxcCreateInstance");
    }
}

DirectXShaderCompilerProvider::~DirectXShaderCompilerProvider()
{
    SystemFreeLibrary(_dxcompilerDll);
}

ShaderLanguage DirectXShaderCompilerProvider::GetShaderLanguage()
{
    return ShaderLanguage_Hlsl;
}

void DirectXShaderCompilerProvider::GetTargetShaderLanguages(ShaderLanguage* targetLanguages, int* targetLanguagesCount)
{
    int32_t index = 0;

    targetLanguages[index++] = ShaderLanguage_Dxil;
    targetLanguages[index++] = ShaderLanguage_Spirv;

    *targetLanguagesCount = index;
}

bool DirectXShaderCompilerProvider::IsCompilerInstalled()
{
    return _createInstanceFunc != nullptr;
}
    
Span<uint8_t> DirectXShaderCompilerProvider::CompileShader(MemoryArena memoryArena, std::vector<ShaderCompilerLogEntry>& logList, std::vector<ShaderMetaData>& metaDataList, Span<uint8_t> shaderCode, ShaderStage shaderStage, uint8_t* entryPoint, ShaderLanguage shaderLanguage, GraphicsApi graphicsApi, ShaderCompilationOptions* options)
{
    SystemAssert(_createInstanceFunc != nullptr);

    auto stackMemoryArena = SystemGetStackMemoryArena();

    ComPtr<IDxcCompiler3> compiler;
    AssertIfFailed(_createInstanceFunc(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler)));

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
    
    // HACK: For the moment we hard code the root signature def
    arguments.push_back(L"-rootsig-define RootSignatureDef");

    // TODO: pass all-resources-bound!!!
    
            
    // TODO: Additional options to be considered
    // Pass a parameter for debug and release mode

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

    ComPtr<IDxcResult> dxilCompileResult;
    AssertIfFailed(compiler->Compile(&sourceBuffer, arguments.data(), (uint32_t)arguments.size(), nullptr, IID_PPV_ARGS(&dxilCompileResult)));

    if (ProcessLogOutput(memoryArena, logList, dxilCompileResult))
    {
        return Span<uint8_t>();
    }
    
    ComPtr<IDxcResult> compileResult;

    if (graphicsApi != GraphicsApi_Direct3D12)
    {
        arguments.push_back(L"-spirv");
        arguments.push_back(L"-fspv-target-env=vulkan1.3");
    
        AssertIfFailed(compiler->Compile(&sourceBuffer, arguments.data(), (uint32_t)arguments.size(), nullptr, IID_PPV_ARGS(&compileResult)));
    }
    else
    {
        compileResult = dxilCompileResult;
    }

    ComPtr<IDxcBlob> shaderByteCodeComPtr;
    AssertIfFailed(compileResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderByteCodeComPtr), nullptr));

    ExtractMetaData(metaDataList, dxilCompileResult);

    auto shaderByteCode = Span<uint8_t>((uint8_t*)shaderByteCodeComPtr->GetBufferPointer(), shaderByteCodeComPtr->GetBufferSize());
    auto outputShaderData = SystemPushArray<uint8_t>(memoryArena, shaderByteCode.Length);
    SystemCopyBuffer<uint8_t>(outputShaderData, shaderByteCode);

    return outputShaderData;
}

bool DirectXShaderCompilerProvider::ProcessLogOutput(MemoryArena memoryArena, std::vector<ShaderCompilerLogEntry>& logList, ComPtr<IDxcResult> compileResult)
{
    // TODO: Allocations are really bad here :(
    auto hasErrors = false;

    // TODO: We cannot use the normal string find methods here because we work directly in UTF8

    ComPtr<IDxcBlobUtf8> pErrors;
    AssertIfFailed(compileResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&pErrors), nullptr));

    if (pErrors && pErrors->GetStringLength() > 0)
    {
        auto currentLogType = ShaderCompilerLogEntryType_Error;

        auto lines = SystemSplitString(memoryArena, (const char*)pErrors->GetBufferPointer(), '\n');
        std::string line;

        for (size_t i = 0; i < lines.Length; i++)
        {
            auto line = lines[i];
            auto lineStd = std::string(lines[i].Pointer); // TODO: To remove

            if (line.Length == 0)
            {
                continue;
            }

            if (lineStd.find("warning:", 0) != std::string::npos)
            {
                currentLogType = ShaderCompilerLogEntryType_Warning;
            }
            else if (lineStd.find("error:", 0) != std::string::npos)
            {
                currentLogType = ShaderCompilerLogEntryType_Error;
                hasErrors = true;
            }
        
            logList.push_back({ currentLogType, (uint8_t*)line.Pointer });
        }
    }

    return hasErrors;
}
    
void DirectXShaderCompilerProvider::ExtractMetaData(std::vector<ShaderMetaData>& metaDataList, ComPtr<IDxcResult> dxilCompileResult)
{
    ComPtr<IDxcUtils> dxcUtils;
    AssertIfFailed(_createInstanceFunc(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils)));
    
    ComPtr<IDxcBlob> shaderReflectionBlob;
    AssertIfFailed(dxilCompileResult->GetOutput(DXC_OUT_REFLECTION, IID_PPV_ARGS(&shaderReflectionBlob), nullptr));

    DxcBuffer reflectionBuffer;
    reflectionBuffer.Ptr = shaderReflectionBlob->GetBufferPointer();
    reflectionBuffer.Size = shaderReflectionBlob->GetBufferSize();
    reflectionBuffer.Encoding = 0;

    ComPtr<ID3D12ShaderReflection> shaderReflection;
    AssertIfFailed(dxcUtils->CreateReflection(&reflectionBuffer, IID_PPV_ARGS(&shaderReflection)));

    uint32_t threadCountX, threadCountY, threadCountZ;
    shaderReflection->GetThreadGroupSize(&threadCountX, &threadCountY, &threadCountZ);

    if (threadCountX > 0 && threadCountY > 0 && threadCountZ > 0)
    {
        metaDataList.push_back({ ShaderMetaDataType_ThreadCountX, threadCountX });
        metaDataList.push_back({ ShaderMetaDataType_ThreadCountY, threadCountY });
        metaDataList.push_back({ ShaderMetaDataType_ThreadCountZ, threadCountZ });
    }

    ComPtr<IDxcBlob> dxilShaderByteCode;
    AssertIfFailed(dxilCompileResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&dxilShaderByteCode), nullptr));

    ComPtr<IDxcContainerReflection> containerReflection;
    AssertIfFailed(_createInstanceFunc(CLSID_DxcContainerReflection, IID_PPV_ARGS(&containerReflection)));

    IDxcBlob* rootSignatureBlob = {};

    #ifdef _WINDOWS
    AssertIfFailed(containerReflection->Load(dxilShaderByteCode.Get()));
    #else
    AssertIfFailed(containerReflection->Load(dxilShaderByteCode));
    #endif

    uint32_t partCount = 0;
    containerReflection->GetPartCount(&partCount);

    for (uint32_t i = 0; i < partCount; i++)
    {
        uint32_t partKind = 0;
        AssertIfFailed(containerReflection->GetPartKind(i, &partKind));

        if (partKind == DXC_PART_ROOT_SIGNATURE)
        {
            AssertIfFailed(containerReflection->GetPartContent(i, &rootSignatureBlob));
        }
    }

    if (rootSignatureBlob)
    {
        auto pointer = (uint8_t*)rootSignatureBlob->GetBufferPointer();
        auto desc = (DxilContainerRootSignatureDesc*)pointer;

        // TODO: Handle multiple parameters
        for (uint32_t i = 0; i < desc->NumParameters; i++)
        {
            auto parameter = (DxilContainerRootParameter*)(pointer + desc->RootParametersOffset + i * sizeof(DxilContainerRootParameter));

            if ((DxilRootParameterType)parameter->ParameterType == DxilRootParameterType::Constants32Bit)
            {
                auto payload = (DxilRootConstants*)(pointer + parameter->PayloadOffset);
                auto pushConstantCount = payload->Num32BitValues;
            
                metaDataList.push_back({ ShaderMetaDataType_PushConstantsCount, pushConstantCount });
                break;
            }
        }
    }
}
