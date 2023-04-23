#include "PreCompiledHeader.h"
#include "ElementalTools.h"
#include "ShaderCompilerProvider.h"
#include "DirectXShaderCompilerProvider.h"
#include "SpirvCrossShaderCompilerProvider.h"
#include "MetalShaderCompilerProvider.h"

// TODO: Do we keep std datastructures? Perf is not critical in the tools
std::vector<ShaderCompilerProvider*> _shaderCompilerProviders;
std::unordered_map<GraphicsApi, ShaderLanguage> _platformTargetLanguages;

void RegisterShaderCompilerProvider(ShaderCompilerProvider* shaderCompilerProvider)
{
    if (!shaderCompilerProvider->IsCompilerInstalled())
    {
        delete shaderCompilerProvider;
        return;
    }
    
    _shaderCompilerProviders.push_back(shaderCompilerProvider);
}

ShaderCompilerProvider* FindShaderCompiler(ShaderLanguage targetShaderLanguage)
{
    for (size_t i = 0; i < _shaderCompilerProviders.size(); i++)
    {
        auto shaderCompilerProvider = _shaderCompilerProviders[i];

        ShaderLanguage targetLanguages[10];
        int targetLanguageCount = 0;

        shaderCompilerProvider->GetTargetShaderLanguages(targetLanguages, &targetLanguageCount);

        for (auto j = 0; j < targetLanguageCount; j++)
        {
            auto providerTargetShaderLanguage = targetLanguages[j];

            if (providerTargetShaderLanguage == targetShaderLanguage)
            {
                return shaderCompilerProvider;
            }
        }
    }

    return nullptr;
}

int FindShaderCompilersChain(ShaderCompilerProvider** shaderCompilers, int32_t currentLevel, ShaderLanguage sourceLanguage, ShaderLanguage targetLanguage)
{
    auto compiler = FindShaderCompiler(targetLanguage);

    if (compiler == nullptr)
    {
        return 0;
    }

    shaderCompilers[currentLevel] = compiler;

    if (compiler->GetShaderLanguage() == sourceLanguage)
    {
        return 1;
    }
    else
    {
        auto result = FindShaderCompilersChain(shaderCompilers, currentLevel + 1, sourceLanguage, compiler->GetShaderLanguage());

        if (result > 0)
        {
            return result + 1;
        }

        return 0;
    }
}

DllExport void Native_FreeNativePointer(void* nativePointer)
{
    free(nativePointer);
}

DllExport void Native_InitShaderCompiler()
{
    #ifdef _DEBUG
    SystemInitDebugAllocations();
    #endif

    _platformTargetLanguages[GraphicsApi_Direct3D12] = ShaderLanguage_Dxil;
    _platformTargetLanguages[GraphicsApi_Vulkan] = ShaderLanguage_Spirv;
    _platformTargetLanguages[GraphicsApi_Metal] = ShaderLanguage_MetalIR;

    RegisterShaderCompilerProvider((ShaderCompilerProvider*)new DirectXShaderCompilerProvider());
    RegisterShaderCompilerProvider((ShaderCompilerProvider*)new SpirvCrossShaderCompilerProvider());
    RegisterShaderCompilerProvider((ShaderCompilerProvider*)new MetalShaderCompilerProvider());
}
    
DllExport void Native_FreeShaderCompiler()
{
    for (size_t i = 0; i < _shaderCompilerProviders.size(); i++)
    {
        delete _shaderCompilerProviders[i];
    }
    
    #ifdef _DEBUG
    SystemCheckAllocations("Elemental Tools");
    #endif
}

DllExport bool Native_CanCompileShader(ShaderLanguage shaderLanguage, GraphicsApi graphicsApi)
{
    if (!_platformTargetLanguages.count(graphicsApi))
    {
        return false;
    }

    auto graphicsApiTargetShaderLanguage = _platformTargetLanguages[graphicsApi];

    ShaderCompilerProvider* shaderCompilerProviders[10];
    auto result = FindShaderCompilersChain(shaderCompilerProviders, 0, shaderLanguage, graphicsApiTargetShaderLanguage);
    return result > 0;
}
    
DllExport ShaderCompilerResult Native_CompileShader(uint8_t* shaderCode, ShaderStage shaderStage, uint8_t* entryPoint, ShaderLanguage shaderLanguage, GraphicsApi graphicsApi, ShaderCompilationOptions* options)
{
    if (!_platformTargetLanguages.count(graphicsApi))
    {
        return CreateErrorResult(shaderStage, entryPoint, SystemConvertWideCharToUtf8(L"Graphics API is not supported!"));
    }

    auto graphicsApiTargetShaderLanguage = _platformTargetLanguages[graphicsApi];

    ShaderCompilerProvider* shaderCompilerProviders[10];
    auto result = FindShaderCompilersChain(shaderCompilerProviders, 0, shaderLanguage, graphicsApiTargetShaderLanguage);

    if (result > 0)
    {
        auto isSuccess = false;
        auto logList = std::vector<ShaderCompilerLogEntry>();
        auto metaDataList = std::vector<ShaderMetaData>();

        auto currentShaderData = Span<uint8_t>(shaderCode, (uint32_t)strlen((char*)shaderCode));

        for (int32_t i = result - 1; i >= 0; i--)
        {
            auto shaderCompilerProvider = shaderCompilerProviders[i];

            auto compilationResult = shaderCompilerProvider->CompileShader(logList, metaDataList, currentShaderData, shaderStage, entryPoint, shaderLanguage, graphicsApi, options);

            isSuccess = !compilationResult.IsEmpty();
            
            if (!isSuccess)
            {
                currentShaderData = Span<uint8_t>::Empty();
                break;
            }

            // BUG: We need to delete output data of intermediate results
            currentShaderData = compilationResult;
        }

        auto logEntriesData = new ShaderCompilerLogEntry[logList.size()];
        memcpy(logEntriesData, logList.data(), logList.size() * sizeof(ShaderCompilerLogEntry));
        
        auto metaDataListData = new ShaderMetaData[metaDataList.size()];
        memcpy(metaDataListData, metaDataList.data(), metaDataList.size() * sizeof(ShaderMetaData));
        
        ShaderCompilerResult compilerResult = {};

        compilerResult.IsSuccess = isSuccess;
        compilerResult.Stage = shaderStage;
        compilerResult.EntryPoint = entryPoint;
        compilerResult.LogEntries = logEntriesData;
        compilerResult.LogEntryCount = (uint32_t)logList.size();
        compilerResult.ShaderData = currentShaderData.Pointer;
        compilerResult.ShaderDataCount = (uint32_t)currentShaderData.Length;
        compilerResult.MetaData = metaDataListData;
        compilerResult.MetaDataCount = (uint32_t)metaDataList.size();

        return compilerResult;
    } 
        
    return CreateErrorResult(shaderStage, entryPoint, SystemConvertWideCharToUtf8(L"Cannot find compatible shader compilers toolchain."));
}
    
DllExport void Native_CompileShaders(ShaderCompilerInput* inputs, int32_t inputCount, GraphicsApi graphicsApi, ShaderCompilationOptions* options, ShaderCompilerResult* results, int32_t* resultCount)
{
    *resultCount = inputCount;

    for (int32_t i = 0; i < inputCount; i++)
    {
        auto input = inputs[i];
        results[i] = Native_CompileShader(input.ShaderCode, input.Stage, input.EntryPoint, input.ShaderLanguage, graphicsApi, options);
    }
}