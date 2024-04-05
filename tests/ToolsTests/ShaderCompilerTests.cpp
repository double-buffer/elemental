#include "ToolsTests.h"
#include "utest.h"

auto hlslTestSource = "struct PSInput \
{ \
	float4 color : COLOR; \
}; \
\
[shader(\"pixel\")]\
float4 PSMain(PSInput input) : SV_TARGET \
{\
	return input.color;\
}";

auto hlslTestSourceError = "struct PSInput \
{ \
	float4 color : COLOR; \
}; \
ERRRORRRRCODE\
[shader(""pixel"")]\
float4 PSMain(PSInput input) : SV_TARGET \
{\
	return input.color;\
}";


// TODO: Those 2 tests will only run on win32 and macos
UTEST(ShaderCompiler, CanCompileShaderTest_HlslToDirectX12) 
{
    // Act
    auto result = ElemCanCompileShader(ElemShaderLanguage_Hlsl, ElemToolsGraphicsApi_DirectX12);

    // Assert
    ASSERT_TRUE(result);
}

UTEST(ShaderCompiler, CanCompileShaderTest_HlslToMetal) 
{
    // Act
    auto result = ElemCanCompileShader(ElemShaderLanguage_Hlsl, ElemToolsGraphicsApi_Metal);

    // Assert
    ASSERT_TRUE(result);
}

UTEST(ShaderCompiler, CanCompileShaderTest_ImpossibleConfigReturnsFalse) 
{
    // Act
    auto result = ElemCanCompileShader(ElemShaderLanguage_Msl, ElemToolsGraphicsApi_DirectX12);

    // Assert
    ASSERT_FALSE(result);
}

struct ShaderCompiler_CompileShader
{
    ElemShaderLanguage SourceLanguage;
    ElemToolsGraphicsApi TargetGraphicsApi;
    bool WithError;
};

UTEST_F_SETUP(ShaderCompiler_CompileShader) 
{
}

UTEST_F_TEARDOWN(ShaderCompiler_CompileShader) 
{
    // Act / Assert
    // TODO: Make a macro to select the correct source code
    auto sourceCode = !utest_fixture->WithError ? hlslTestSource : hlslTestSourceError;

    ElemShaderSourceData sourceData =
    {
        .ShaderLanguage = utest_fixture->SourceLanguage, 
        .Data = 
        { 
            .Items = (uint8_t*)sourceCode, 
            .Length = (uint32_t)strlen(sourceCode)
        }
    };

    auto compilationResult = ElemCompileShaderLibrary(utest_fixture->TargetGraphicsApi, &sourceData, nullptr);
    auto errorCount = 0u;

    for (uint32_t i = 0; i < compilationResult.Messages.Length; i++)
    {
        if (compilationResult.Messages.Items[i].Type == ElemToolsMessageType_Error)
        {
            errorCount++;
        }
    }

    if (!utest_fixture->WithError)
    {
        ASSERT_FALSE_MSG(compilationResult.HasErrors, "Compilation contains errors");
        ASSERT_EQ_MSG(0u, errorCount, "Compilation contains errors");
        ASSERT_LT_MSG(0u, compilationResult.Data.Length, "Compilation data is empty");
    }
    else
    {
        ASSERT_TRUE_MSG(compilationResult.HasErrors, "Compilation contains errors");
        ASSERT_LT_MSG(0u, errorCount, "Compilation contains errors");
        ASSERT_EQ_MSG(0u, compilationResult.Data.Length, "Compilation data is empty");
    }
}

UTEST_F(ShaderCompiler_CompileShader, Hlsl_DirectX12) 
{
    utest_fixture->SourceLanguage = ElemShaderLanguage_Hlsl;
    utest_fixture->TargetGraphicsApi = ElemToolsGraphicsApi_DirectX12;
    utest_fixture->WithError = false;
}

UTEST_F(ShaderCompiler_CompileShader, Hlsl_DirectX12_Error) 
{
    utest_fixture->SourceLanguage = ElemShaderLanguage_Hlsl;
    utest_fixture->TargetGraphicsApi = ElemToolsGraphicsApi_DirectX12;
    utest_fixture->WithError = true;
}

UTEST_F(ShaderCompiler_CompileShader, Hlsl_Metal) 
{
    utest_fixture->SourceLanguage = ElemShaderLanguage_Hlsl;
    utest_fixture->TargetGraphicsApi = ElemToolsGraphicsApi_Metal;
    utest_fixture->WithError = false;
}

UTEST_F(ShaderCompiler_CompileShader, Hlsl_Metal_Error) 
{
    utest_fixture->SourceLanguage = ElemShaderLanguage_Hlsl;
    utest_fixture->TargetGraphicsApi = ElemToolsGraphicsApi_Metal;
    utest_fixture->WithError = true;
}

// TODO: Test multiple config like HLSL => SPIRV => METALIR
