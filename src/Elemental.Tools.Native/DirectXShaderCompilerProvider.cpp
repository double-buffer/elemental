#include "DirectXShaderCompilerProvider.h"
#include <vector>

DirectXShaderCompilerProvider::DirectXShaderCompilerProvider()
{
#ifdef _WINDOWS
    const wchar_t* dllName = L"./dxcompiler.dll";
#elif __APPLE__
    const wchar_t* dllName = L"libdxcompiler.dylib";
#else
    const wchar_t* dllName = L"libdxcompiler.so";
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
                _createInstanceFunc = (DxcCreateInstanceProc)::dlsym(m_dxcompilerDll, functionName);
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
    
ShaderCompilerResult DirectXShaderCompilerProvider::CompileShader(uint8_t* shaderCode, uint32_t shaderCodeSize, ToolsShaderStage shaderStage, uint8_t* entryPoint, ShaderLanguage shaderLanguage, ToolsGraphicsApi graphicsApi)
{
    assert(_createInstanceFunc != nullptr);

    ComPtr<IDxcCompiler3> compiler;
    AssertIfFailed(_createInstanceFunc(CLSID_DxcCompiler, __uuidof(IDxcCompiler3), reinterpret_cast<void**>(compiler.GetAddressOf())));

    ComPtr<IDxcUtils> dxcUtils;
    AssertIfFailed(_createInstanceFunc(CLSID_DxcUtils, IID_PPV_ARGS(dxcUtils.GetAddressOf())));

    ComPtr<IDxcBlobEncoding> sourceBlob;
    AssertIfFailed(dxcUtils->CreateBlob(shaderCode, shaderCodeSize, CP_UTF8, sourceBlob.GetAddressOf()));

    std::vector<LPCWSTR> arguments;

    arguments.push_back(L"-HV 2021");

    //-E for the entry point (eg. PSMain)
    arguments.push_back(L"-E");
    
    auto entryPointString = ConvertUtf8ToWString(entryPoint);
    arguments.push_back(entryPointString.c_str());

    // TODO: Use defines
    auto shaderTarget = L"ms_6_7";

    if (shaderStage == ToolsShaderStage_AmplificationShader)
    {
        shaderTarget = L"as_6_7";
    }
    else if (shaderStage == ToolsShaderStage_PixelShader)
    {
        shaderTarget = L"ps_6_7";
    }

    arguments.push_back(L"-T");
    arguments.push_back(shaderTarget);
    
    if (graphicsApi != ToolsGraphicsApi_Direct3D12)
    {
        arguments.push_back(L"-spirv");
        arguments.push_back(L"-fspv-target-env=vulkan1.3");
    }
    else
    {
        // HACK: For the moment we hard code the root signature def
        arguments.push_back(L"-rootsig-define RootSignatureDef");
    }
            
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
    AssertIfFailed(compiler->Compile(&sourceBuffer, arguments.data(), (uint32_t)arguments.size(), nullptr, IID_PPV_ARGS(compileResult.GetAddressOf())));

    auto logList = std::vector<ShaderCompilerLogEntry>();
    auto hasErrors = false;

    ComPtr<IDxcBlobUtf8> pErrors;
    compileResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(pErrors.GetAddressOf()), nullptr);

    if (pErrors && pErrors->GetStringLength() > 0)
    {
        auto errorContent = ConvertUtf8ToWString((uint8_t*)pErrors->GetBufferPointer()); // std::wstring((wchar_t*)pErrors->GetBufferPointer(), pErrors->GetStringLength());
        std::wstring line;
        auto currentLogType = ShaderCompilerLogEntryType_Error;

        // TODO: Process lines

        ShaderCompilerLogEntry logEntry = {};
        logEntry.Type = ShaderCompilerLogEntryType_Error;
        logEntry.Message = ConvertWStringToUtf8(errorContent);
        //logList.push_back(logEntry);

/*
    while ((line = process.StandardError.ReadLine()) != null)
    {
        if (line.Contains("warning:"))
        {
            currentLogType = ShaderCompilerLogEntryType.Warning;
        }
        else if (line.Contains("error:"))
        {
            currentLogType = ShaderCompilerLogEntryType.Error;
            hasErrors = true;
        }

        auto logEntry = CreateErrorResult(shaderStage, entryPoint, ConvertWStringToUtf8());
        logList.Add(new() { Type = currentLogType, Message = line });
    }
    
    while ((line = process.StandardOutput.ReadLine()) != null)
    {
        logList.Add(new() { Type = ShaderCompilerLogEntryType.Message, Message = line });
    }*/
    }

    ComPtr<IDxcBlob> shaderByteCode;
    AssertIfFailed(compileResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(shaderByteCode.GetAddressOf()), nullptr));
    
    auto outputShaderData = new uint8_t[shaderByteCode->GetBufferSize()];
    memcpy(outputShaderData, shaderByteCode->GetBufferPointer(), shaderByteCode->GetBufferSize());

    ShaderCompilerResult result = {};

    result.IsSuccess = !hasErrors;
    result.Stage = shaderStage;
    result.EntryPoint = entryPoint;
    result.ShaderData = outputShaderData;
    result.ShaderDataCount = shaderByteCode->GetBufferSize();
    result.LogEntries = logList.data();
    result.LogEntryCount = logList.size();

    return result;
}