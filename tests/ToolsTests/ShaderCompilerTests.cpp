#include "ToolsTests.h"
#include "utest.h"

auto hlslTestSource = "struct PSInput \
{ \
	float4 color : COLOR; \
}; \
\
float4 PSMain(PSInput input) : SV_TARGET \
{\
	return input.color;\
}";

auto hlslTestSourceError = "struct PSInput \
{ \
	float4 color : COLOR; \
}; \
ERRRORRRRCODE\
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

UTEST(ShaderCompiler, CompileShader_Hlsl_DirectX12) 
{
    // Act / Assert
    auto canCompileShader = ElemCanCompileShader(ElemShaderLanguage_Hlsl, ElemToolsGraphicsApi_DirectX12);

    if (canCompileShader)
    {
        ElemShaderSourceData sourceData =
            {
                .ShaderLanguage = ElemShaderLanguage_Hlsl, 
                .Data = 
                { 
                    .Items = (uint8_t*)hlslTestSource, 
                    .Length = (uint32_t)strlen(hlslTestSource)
                }
            };

        auto compilationResult = ElemCompileShaderLibrary(ElemToolsGraphicsApi_DirectX12, &sourceData, nullptr);
        auto errorCount = 0u;

        for (uint32_t i = 0; i < compilationResult.Messages.Length; i++)
        {
            if (compilationResult.Messages.Items[i].Type == ElemToolsMessageType_Error)
            {
                errorCount++;
            }
        }

        ASSERT_FALSE_MSG(compilationResult.HasErrors, "Compilation contains errors");
        ASSERT_EQ_MSG(0u, errorCount, "Compilation contains errors");
        ASSERT_LT_MSG(0u, compilationResult.Data.Length, "Compilation data is empty");
    }
}

UTEST(ShaderCompiler, CompileShader_Hlsl_DirectX12_Error) 
{
    // Act / Assert
    auto canCompileShader = ElemCanCompileShader(ElemShaderLanguage_Hlsl, ElemToolsGraphicsApi_DirectX12);

    if (canCompileShader)
    {
        ElemShaderSourceData sourceData =
            {
                .ShaderLanguage = ElemShaderLanguage_Hlsl, 
                .Data = 
                { 
                    .Items = (uint8_t*)hlslTestSourceError, 
                    .Length = (uint32_t)strlen(hlslTestSourceError)
                }
            };

        auto compilationResult = ElemCompileShaderLibrary(ElemToolsGraphicsApi_DirectX12, &sourceData, nullptr);
        auto errorCount = 0u;

        for (uint32_t i = 0; i < compilationResult.Messages.Length; i++)
        {
            if (compilationResult.Messages.Items[i].Type == ElemToolsMessageType_Error)
            {
                errorCount++;
            }
        }

        ASSERT_TRUE_MSG(compilationResult.HasErrors, "Compilation contains errors");
        ASSERT_LT_MSG(0u, errorCount, "Compilation contains errors");
        ASSERT_EQ_MSG(0u, compilationResult.Data.Length, "Compilation data is empty");
    }
}

// TODO: Test multiple config like HLSL => SPIRV => METALIR
