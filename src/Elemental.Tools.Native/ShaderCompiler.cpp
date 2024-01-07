#include "ShaderCompiler.h"

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

DllExport void Native_InitShaderCompiler(ShaderCompilerOptions* options)
{
    if (options->LogMessageHandler)
    {
        SystemRegisterLogMessageHandler(options->LogMessageHandler);
    }

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

ShaderCompilerResult CompileShader(MemoryArena memoryArena, uint8_t* shaderCode, ShaderStage shaderStage, uint8_t* entryPoint, ShaderLanguage shaderLanguage, GraphicsApi graphicsApi, ShaderCompilationOptions* options)
{
    if (!_platformTargetLanguages.count(graphicsApi))
    {
        return CreateErrorResult(shaderStage, entryPoint, (uint8_t*)"Graphics API is not supported!");
    }

    auto graphicsApiTargetShaderLanguage = _platformTargetLanguages[graphicsApi];

    ShaderCompilerProvider* shaderCompilerProviders[10];
    auto result = FindShaderCompilersChain(shaderCompilerProviders, 0, shaderLanguage, graphicsApiTargetShaderLanguage);

    if (result > 0)
    {
        auto isSuccess = false;
        auto logList = std::vector<ShaderCompilerLogEntry>(); // TODO: To Remove
        auto metaDataList = std::vector<ShaderMetaData>();

        auto currentShaderData = Span<uint8_t>(shaderCode, (uint32_t)strlen((char*)shaderCode));

        for (int32_t i = result - 1; i >= 0; i--)
        {
            auto shaderCompilerProvider = shaderCompilerProviders[i];

            auto compilationResult = shaderCompilerProvider->CompileShader(memoryArena, logList, metaDataList, currentShaderData, shaderStage, entryPoint, shaderLanguage, graphicsApi, options);

            isSuccess = compilationResult.Length > 0;
            
            if (!isSuccess)
            {
                currentShaderData = Span<uint8_t>();
                break;
            }

            currentShaderData = compilationResult;
        }

        auto logListSpan = Span<ShaderCompilerLogEntry>(logList.data(), logList.size());
        auto logEntriesData = SystemPushArray<ShaderCompilerLogEntry>(memoryArena, logList.size());
        SystemCopyBuffer<ShaderCompilerLogEntry>(logEntriesData, logListSpan);

        auto metaDataListData = new ShaderMetaData[metaDataList.size()];
        memcpy(metaDataListData, metaDataList.data(), metaDataList.size() * sizeof(ShaderMetaData));

        ShaderCompilerResult compilerResult = {};

        compilerResult.IsSuccess = isSuccess;
        compilerResult.Stage = shaderStage;
        compilerResult.EntryPoint = entryPoint;
        compilerResult.LogEntries = logEntriesData.Pointer;
        compilerResult.LogEntryCount = logEntriesData.Length;
        compilerResult.ShaderData = currentShaderData.Pointer;
        compilerResult.ShaderDataCount = (uint32_t)currentShaderData.Length;
        compilerResult.MetaData = metaDataListData;
        compilerResult.MetaDataCount = (uint32_t)metaDataList.size();

        return compilerResult;
    } 
        
    return CreateErrorResult(shaderStage, entryPoint, (uint8_t*)"Cannot find compatible shader compilers toolchain.");
}

// TODO: Why do we use utf 8? all strings needs to be copied and encoded
DllExport ShaderCompilerResult Native_CompileShader(uint8_t* shaderCode, ShaderStage shaderStage, uint8_t* entryPoint, ShaderLanguage shaderLanguage, GraphicsApi graphicsApi, ShaderCompilationOptions* options)
{
    auto memoryArena = SystemGetStackMemoryArena();
    return CompileShader(memoryArena, shaderCode, shaderStage, entryPoint, shaderLanguage, graphicsApi, options);
}

DllExport void Native_CompileShaders(ShaderCompilerInput* inputs, int32_t inputCount, GraphicsApi graphicsApi, ShaderCompilationOptions* options, ShaderCompilerResult* results, int32_t* resultCount)
{
    auto memoryArena = SystemGetStackMemoryArena();
    *resultCount = inputCount;

    for (int32_t i = 0; i < inputCount; i++)
    {
        auto input = inputs[i];
        results[i] = CompileShader(memoryArena, input.ShaderCode, input.Stage, input.EntryPoint, input.ShaderLanguage, graphicsApi, options);
    }
}

const ShaderCompilerResult CreateErrorResult(ShaderStage stage, const uint8_t* entryPoint, const uint8_t* message)
{
    ShaderCompilerResult result = {};

    result.IsSuccess = false;
    result.Stage = stage;
    result.EntryPoint = entryPoint;
    result.LogEntries = new ShaderCompilerLogEntry[1];
    result.LogEntries[0].Type = ShaderCompilerLogEntryType_Error;
    result.LogEntries[0].Message = message;
    result.LogEntryCount = 1;

    return result;
}
