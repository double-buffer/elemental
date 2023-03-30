#include "DirectXShaderCompilerProvider.h"

DirectXShaderCompilerProvider::DirectXShaderCompilerProvider()
{
    _dxcompilerDll = SystemLoadLibrary("dxcompiler");

    if (_dxcompilerDll != nullptr)
    {
        _createInstanceFunc = (DxcCreateInstanceProc)SystemGetFunctionExport(_dxcompilerDll, "DxcCreateInstance");
        
        if (_createInstanceFunc)
        {
            AssertIfFailed(_createInstanceFunc(CLSID_DxcUtils, IID_PPV_ARGS(&_dxcUtils)));
        }
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
    
Span<uint8_t> DirectXShaderCompilerProvider::CompileShader(std::vector<ShaderCompilerLogEntry>& logList, std::vector<ShaderMetaData>& metaDataList, Span<uint8_t> shaderCode, ShaderStage shaderStage, uint8_t* entryPoint, ShaderLanguage shaderLanguage, GraphicsApi graphicsApi, ShaderCompilationOptions* options)
{
    assert(_createInstanceFunc != nullptr);

    ComPtr<IDxcCompiler3> compiler;
    AssertIfFailed(_createInstanceFunc(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler)));

    // TODO: Use utils function to build parameters
    std::vector<const wchar_t*> arguments;
    arguments.push_back(L"-HV 2021");

    //-E for the entry point (eg. PSMain)
    arguments.push_back(L"-E");
    
    auto entryPointString = SystemConvertUtf8ToWString(entryPoint);
    arguments.push_back(entryPointString.c_str());

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

    auto hasErrors = ProcessLogOutput(logList, compileResult);
    
    ComPtr<IDxcBlob> shaderByteCode;
    AssertIfFailed(compileResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderByteCode), nullptr));

    ExtractMetaData(metaDataList, dxilCompileResult);

    auto outputShaderData = new uint8_t[shaderByteCode->GetBufferSize()];
    memcpy(outputShaderData, shaderByteCode->GetBufferPointer(), shaderByteCode->GetBufferSize());

    return Span<uint8_t>(outputShaderData, shaderByteCode->GetBufferSize());
}

bool DirectXShaderCompilerProvider::ProcessLogOutput(std::vector<ShaderCompilerLogEntry>& logList, ComPtr<IDxcResult> compileResult)
{
    auto hasErrors = false;
    auto numOutputs = compileResult->GetNumOutputs();

    ComPtr<IDxcBlobUtf8> pErrors;
    AssertIfFailed(compileResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&pErrors), nullptr));

    if (pErrors && pErrors->GetStringLength() > 0)
    {
        auto errorContent = SystemConvertUtf8ToWString((uint8_t*)pErrors->GetBufferPointer());
        auto currentLogType = ShaderCompilerLogEntryType_Error;

        auto lines = SystemSplitString(errorContent, L"\n");
        std::wstring line;

        for (int32_t i = 0; i < lines.size(); i++)
        {
            line = lines[i];

            if (line.length() == 0)
            {
                continue;
            }

            if (line.find(L"warning:", 0) != -1)
            {
                currentLogType = ShaderCompilerLogEntryType_Warning;
            }
            else if (line.find(L"error:", 0) != -1)
            {
                currentLogType = ShaderCompilerLogEntryType_Error;
                hasErrors = true;
            }
            
            logList.push_back({ currentLogType, SystemConvertWStringToUtf8(line) });
        }
    }

    return hasErrors;
}
    
void DirectXShaderCompilerProvider::ExtractMetaData(std::vector<ShaderMetaData>& metaDataList, ComPtr<IDxcResult> dxilCompileResult)
{
    ComPtr<IDxcBlob> shaderReflectionBlob;
    AssertIfFailed(dxilCompileResult->GetOutput(DXC_OUT_REFLECTION, IID_PPV_ARGS(&shaderReflectionBlob), nullptr));

    DxcBuffer reflectionBuffer;
    reflectionBuffer.Ptr = shaderReflectionBlob->GetBufferPointer();
    reflectionBuffer.Size = shaderReflectionBlob->GetBufferSize();
    reflectionBuffer.Encoding = 0;

    ComPtr<ID3D12ShaderReflection> shaderReflection;
    AssertIfFailed(_dxcUtils->CreateReflection(&reflectionBuffer, IID_PPV_ARGS(&shaderReflection)));

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

    IDxcBlob* rootSignatureBlob;

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

    auto pointer = (uint8_t*)rootSignatureBlob->GetBufferPointer();

    auto desc = (DxilContainerRootSignatureDesc*)pointer;

    // TODO: Handle multiple parameters
    for (int i = 0; i < desc->NumParameters; i++)
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