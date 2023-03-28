#include "ElementalTools.h"
#include "ShaderCompilerProvider.h"
#include "DirectXShaderCompilerProvider.h"
#include "SpirvCrossShaderCompilerProvider.h"
#include "MetalShaderCompilerProvider.h"

// TODO: Do we keep std datastructures? Perf is not critical in the tools
#include <vector>
#include <unordered_map>

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
    for (auto i = 0; i < _shaderCompilerProviders.size(); i++)
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
    _platformTargetLanguages[GraphicsApi_Direct3D12] = ShaderLanguage_Dxil;
    _platformTargetLanguages[GraphicsApi_Vulkan] = ShaderLanguage_Spirv;
    _platformTargetLanguages[GraphicsApi_Metal] = ShaderLanguage_MetalIR;

    RegisterShaderCompilerProvider((ShaderCompilerProvider*)new DirectXShaderCompilerProvider());
    RegisterShaderCompilerProvider((ShaderCompilerProvider*)new SpirvCrossShaderCompilerProvider());
    RegisterShaderCompilerProvider((ShaderCompilerProvider*)new MetalShaderCompilerProvider());
}
    
DllExport void Native_FreeShaderCompiler()
{
    for (auto i = 0; i < _shaderCompilerProviders.size(); i++)
    {
        delete _shaderCompilerProviders[i];
    }
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
    
DllExport ShaderCompilerResult Native_CompileShader(uint8_t* shaderCode, ShaderStage shaderStage, uint8_t* entryPoint, ShaderLanguage shaderLanguage, GraphicsApi graphicsApi)
{
    // TODO: Review memory management because we copy 2 times per level :(

    // TODO: Review STD structures 
    if (!_platformTargetLanguages.count(graphicsApi))
    {
        return CreateErrorResult(shaderStage, entryPoint, ConvertWStringToUtf8(L"Graphics API is not supported!"));
    }

    auto graphicsApiTargetShaderLanguage = _platformTargetLanguages[graphicsApi];

    ShaderCompilerProvider* shaderCompilerProviders[10];
    auto result = FindShaderCompilersChain(shaderCompilerProviders, 0, shaderLanguage, graphicsApiTargetShaderLanguage);

    if (result > 0)
    {
        auto isSuccess = false;
        auto logList = std::vector<ShaderCompilerLogEntry>();
        auto metaDataList = std::vector<ShaderMetaData>();

        uint8_t* currentShaderData = nullptr;
        uint32_t currentShaderDataCount = 0;

        uint8_t* currentShaderInput = shaderCode;
        uint32_t currentShaderInputSize = strlen((char*)shaderCode);

        for (int32_t i = result - 1; i >= 0; i--)
        {
            auto shaderCompilerProvider = shaderCompilerProviders[i];

            auto compilationResult = shaderCompilerProvider->CompileShader(currentShaderInput, currentShaderInputSize, shaderStage, entryPoint, shaderLanguage, graphicsApi);

            for (uint32_t j = 0; j < compilationResult.LogEntryCount; j++)
            {
                logList.push_back(compilationResult.LogEntries[j]);
            }
            
            for (uint32_t j = 0; j < compilationResult.MetaDataCount; j++)
            {
                metaDataList.push_back(compilationResult.MetaData[j]);
            }
            
            isSuccess = compilationResult.IsSuccess;
            
            if (!isSuccess)
            {
                currentShaderData = nullptr;
                currentShaderDataCount = 0;
                break;
            }

            // BUG: We need to delete output data of intermediate results
            currentShaderData = compilationResult.ShaderData;
            currentShaderDataCount = compilationResult.ShaderDataCount;

            currentShaderInput = currentShaderData;
            currentShaderInputSize = currentShaderDataCount;
        }

        auto logEntriesData = new ShaderCompilerLogEntry[logList.size()];
        memcpy(logEntriesData, logList.data(), logList.size() * sizeof(ShaderCompilerLogEntry));
        
        auto metaDataListData = new ShaderMetaData[metaDataList.size()];
        memcpy(metaDataListData, metaDataList.data(), metaDataList.size() * sizeof(ShaderMetaData));
        
        ShaderCompilerResult result = {};

        result.IsSuccess = isSuccess;
        result.Stage = shaderStage;
        result.EntryPoint = entryPoint;
        result.LogEntries = logEntriesData;
        result.LogEntryCount = logList.size();
        result.ShaderData = currentShaderData;
        result.ShaderDataCount = currentShaderDataCount;
        result.MetaData = metaDataListData;
        result.MetaDataCount = metaDataList.size();

        return result;
    } 
        
    return CreateErrorResult(shaderStage, entryPoint, ConvertWStringToUtf8(L"Cannot find compatible shader compilers toolchain."));
}