#include "ToolsTests.h"
#include "utest.h"

UTEST(ShaderCompiler, CanCompileShaderTest_HlslToDxil) 
{
    // Act
    auto result = ElemCanCompileShader(ElemShaderLanguage_Hlsl, ElemToolsGraphicsApi_DirectX12);

    // Assert
    ASSERT_TRUE(result);
}

// TODO: Test multiple config like HLSL => SPIRV => METALIR
