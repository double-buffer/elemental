#pragma once
#include "PrecompiledHeader.h"
#include "ElementalTools.h"
#include "ShaderCompilerProvider.h"

#include "Interop/DxilRootSignature.h"

class DirectXShaderCompilerProvider : ShaderCompilerProvider
{
public:
    DirectXShaderCompilerProvider();
    ~DirectXShaderCompilerProvider() override;

    ShaderLanguage GetShaderLanguage() override;
    void GetTargetShaderLanguages(ShaderLanguage* targetLanguages, int* targetLanguagesCount) override;

    bool IsCompilerInstalled() override;
    Span<uint8_t> CompileShader(std::vector<ShaderCompilerLogEntry>& logList, std::vector<ShaderMetaData>& metaDataList, Span<uint8_t> shaderCode, ShaderStage shaderStage, uint8_t* entryPoint, ShaderLanguage shaderLanguage, GraphicsApi graphicsApi, ShaderCompilationOptions* options) override;

private:
    const void* _dxcompilerDll = nullptr;
    DxcCreateInstanceProc _createInstanceFunc = nullptr;
    
    bool ProcessLogOutput(std::vector<ShaderCompilerLogEntry>& logList, ComPtr<IDxcResult> compileResult);
    void ExtractMetaData(std::vector<ShaderMetaData>& metaDataList, ComPtr<IDxcResult> dxilCompileResult);
};