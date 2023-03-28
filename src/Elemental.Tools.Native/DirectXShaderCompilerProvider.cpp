#include "DirectXShaderCompilerProvider.h"

DirectXShaderCompilerProvider::DirectXShaderCompilerProvider()
{
    // TODO: Add system functions for that
#ifdef _WINDOWS
    const wchar_t* dllName = L"./dxcompiler.dll";
#elif __APPLE__
    const char* dllName = "./libdxcompiler.dylib";
#else
    const wchar_t* dllName = L"./libdxcompiler.so";
#endif
    const char* functionName = "DxcCreateInstance";

#ifdef _WINDOWS
    _dxcompilerDll = ::LoadLibrary(dllName);
#else
    _dxcompilerDll = ::dlopen(dllName, RTLD_LAZY);
#endif

    if (_dxcompilerDll != nullptr)
    {
        #ifdef _WINDOWS
                _createInstanceFunc = (DxcCreateInstanceProc)::GetProcAddress(_dxcompilerDll, functionName);
#else
                _createInstanceFunc = (DxcCreateInstanceProc)::dlsym(_dxcompilerDll, functionName);
#endif
    }
}

DirectXShaderCompilerProvider::~DirectXShaderCompilerProvider()
{
    #ifdef _WIN32
                FreeLibrary(_dxcompilerDll);
#else
                ::dlclose(_dxcompilerDll);
#endif
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
    
ShaderCompilerResult DirectXShaderCompilerProvider::CompileShader(uint8_t* shaderCode, uint32_t shaderCodeSize, ShaderStage shaderStage, uint8_t* entryPoint, ShaderLanguage shaderLanguage, GraphicsApi graphicsApi)
{
    // HACK: Review all memory allocations: too much copy!

    assert(_createInstanceFunc != nullptr);

    ComPtr<IDxcCompiler3> compiler;
    AssertIfFailed(_createInstanceFunc(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler)));

    ComPtr<IDxcUtils> dxcUtils;
    AssertIfFailed(_createInstanceFunc(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils)));


    ComPtr<IDxcBlobEncoding> sourceBlob;
    AssertIfFailed(dxcUtils->CreateBlob(shaderCode, shaderCodeSize, CP_UTF8, &sourceBlob));

    // TODO: Use utils function to build parameters
    std::vector<const wchar_t*> arguments;
    arguments.push_back(L"-HV 2021");

    //-E for the entry point (eg. PSMain)
    arguments.push_back(L"-E");
    
    auto entryPointString = ConvertUtf8ToWString(entryPoint);
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
    sourceBuffer.Ptr = sourceBlob->GetBufferPointer();
    sourceBuffer.Size = sourceBlob->GetBufferSize();
    sourceBuffer.Encoding = 0;

    ComPtr<IDxcResult> compileResult;
    ComPtr<IDxcResult> dxilCompileResult;
    AssertIfFailed(compiler->Compile(&sourceBuffer, arguments.data(), (uint32_t)arguments.size(), nullptr, IID_PPV_ARGS(&dxilCompileResult)));
    
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

    auto logList = std::vector<ShaderCompilerLogEntry>();
    auto hasErrors = false;

    auto numOutputs = compileResult->GetNumOutputs();

    ComPtr<IDxcBlobUtf8> pErrors;
    AssertIfFailed(compileResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&pErrors), nullptr));

    if (pErrors && pErrors->GetStringLength() > 0)
    {
        auto errorContent = ConvertUtf8ToWString((uint8_t*)pErrors->GetBufferPointer()); // std::wstring((wchar_t*)pErrors->GetBufferPointer(), pErrors->GetStringLength());
        auto currentLogType = ShaderCompilerLogEntryType_Error;

        auto lines = splitString(errorContent, L"\n");
        std::wstring line;

        for (int32_t i = 0; i < lines.size(); i++)
        {
            line = lines[i];

            if (line.find(L"warning:", 0) != -1)
            {
                currentLogType = ShaderCompilerLogEntryType_Warning;
            }
            else if (line.find(L"error:", 0) != -1)
            {
                currentLogType = ShaderCompilerLogEntryType_Error;
                hasErrors = true;
            }
            
            ShaderCompilerLogEntry logEntry = {};
            logEntry.Type = currentLogType;
            logEntry.Message = ConvertWStringToUtf8(line);
            logList.push_back(logEntry);
        }
    }
    
    ComPtr<IDxcBlob> shaderByteCode;
    AssertIfFailed(compileResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderByteCode), nullptr));

    std::vector<ShaderMetaData> metaDataList;

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

    auto outputShaderData = new uint8_t[shaderByteCode->GetBufferSize()];
    memcpy(outputShaderData, shaderByteCode->GetBufferPointer(), shaderByteCode->GetBufferSize());

    auto logEntriesData = new ShaderCompilerLogEntry[logList.size()];
    memcpy(logEntriesData, logList.data(), logList.size() * sizeof(ShaderCompilerLogEntry));
    
    auto metaDataListData = new ShaderMetaData[metaDataList.size()];
    memcpy(metaDataListData, metaDataList.data(), metaDataList.size() * sizeof(ShaderMetaData));

    ShaderCompilerResult result = {};

    result.IsSuccess = !hasErrors;
    result.Stage = shaderStage;
    result.EntryPoint = entryPoint;
    result.ShaderData = outputShaderData;
    result.ShaderDataCount = shaderByteCode->GetBufferSize();
    result.LogEntries = logEntriesData;
    result.LogEntryCount = logList.size();
    result.MetaData = metaDataListData;
    result.MetaDataCount = metaDataList.size();

    return result;
}