#include "ToolsTests.h"
#include "utest.h"

enum ShaderType
{
    ShaderType_Unknown = 0,
    ShaderType_Amplification = 1,
    ShaderType_Mesh = 2,
    ShaderType_Pixel = 3,
    ShaderType_Compute = 4,
    ShaderType_Library = 5
};

enum ShaderMetadataType
{
    ShaderMetadataType_Unknown = 0,
    ShaderMetadataType_ThreadGroupSize = 1
};

auto hlslTestSource = R"(
    [shader("compute")]
    [numthreads(16, 16, 1)]
    void TestCompute(uint3 threadId: SV_DispatchThreadID)
    {
    }

    struct AmplificationPayload
    {
        float Test;
    };

    groupshared AmplificationPayload sharedPayload;

    [shader("amplification")]
    [numthreads(1, 1, 1)] 
    void TestAmplification(in uint3 groupID : SV_GroupID)    
    { 
        DispatchMesh(1, 1, 1, sharedPayload);
    }

    struct VertexOutput
    {
        float4 Position: SV_Position;
    };

    [shader("mesh")]
    [OutputTopology("triangle")]
    [NumThreads(32, 1, 1)]
    void TestMesh(in uint groupThreadId : SV_GroupThreadID, out vertices VertexOutput vertices[3], out indices uint3 indices[1])
    {
        SetMeshOutputCounts(1, 1);
        vertices[groupThreadId].Position = float4(1, 1, 1, 1);
    }

    struct PixelInput
    {
        float4 color : COLOR;
    };

    [shader("pixel")]
    float4 TestPixel(PixelInput input) : SV_TARGET
    {
        return input.color;
    }
)";

auto hlslTestSourceError = R"(
    struct PSInput
    {
        float4 color : COLOR;
    };
    ERRRORRRRCODE
    [shader(""pixel"")]
    float4 PSMain(PSInput input) : SV_TARGET
    {
        return input.color;
    }
)";

UTEST(ShaderCompiler, CanCompileShaderTest_HlslToDirectX12) 
{
    // Act
    auto result = ElemCanCompileShader(ElemShaderLanguage_Hlsl, ElemToolsGraphicsApi_DirectX12, ElemToolsPlatform_Windows);

    // Assert
    ASSERT_TRUE(result);
}

#ifndef __linux__
UTEST(ShaderCompiler, CanCompileShaderTest_HlslToMetal) 
{
    // Act
    auto result = ElemCanCompileShader(ElemShaderLanguage_Hlsl, ElemToolsGraphicsApi_Metal, ElemToolsPlatform_MacOS);

    // Assert
    ASSERT_TRUE(result);
}
#endif

UTEST(ShaderCompiler, CanCompileShaderTest_ImpossibleConfigReturnsFalse) 
{
    // Act
    auto result = ElemCanCompileShader(ElemShaderLanguage_Msl, ElemToolsGraphicsApi_DirectX12, ElemToolsPlatform_Windows);

    // Assert
    ASSERT_FALSE(result);
}

struct ShaderMetadata
{
    ShaderMetadataType Type;
    uint32_t Value[4];
};

struct ShaderInfo
{
    ShaderType Type;
    const char* Name;
    ShaderMetadata Metadata[16];
    uint32_t MetaDataCount;
};

struct ShaderCompiler_CompileShader
{
    ElemShaderLanguage SourceLanguage;
    ElemToolsGraphicsApi TargetGraphicsApi;
    ElemToolsPlatform TargetPlatform;
    ShaderInfo ExpectedShaders[16];
    uint32_t ExpectedShaderCount;
    bool WithError;
};

UTEST_F_SETUP(ShaderCompiler_CompileShader) 
{
    utest_fixture->ExpectedShaderCount = 0;

    utest_fixture->ExpectedShaders[utest_fixture->ExpectedShaderCount++] = 
    { 
        .Type = ShaderType_Compute,
        .Name = "TestCompute",
        .Metadata = {
        {
            .Type = ShaderMetadataType_ThreadGroupSize,
            .Value = { 16u, 16u, 1u, 0u }
        }},
        .MetaDataCount = 1
    };

    utest_fixture->ExpectedShaders[utest_fixture->ExpectedShaderCount++] = 
    { 
        .Type = ShaderType_Amplification,
        .Name = "TestAmplification",
        .Metadata = {
        {
            .Type = ShaderMetadataType_ThreadGroupSize,
            .Value = { 1u, 1u, 1u, 0u }
        }},
        .MetaDataCount = 1
    };

    utest_fixture->ExpectedShaders[utest_fixture->ExpectedShaderCount++] = 
    { 
        .Type = ShaderType_Mesh,
        .Name = "TestMesh",
        .Metadata = {
        {
            .Type = ShaderMetadataType_ThreadGroupSize,
            .Value = { 32u, 1u, 1u, 0u }
        }},
        .MetaDataCount = 1
    };

    utest_fixture->ExpectedShaders[utest_fixture->ExpectedShaderCount++] = 
    { 
        .Type = ShaderType_Pixel,
        .Name = "TestPixel",
        .Metadata = {
        {
            .Type = ShaderMetadataType_ThreadGroupSize,
            .Value = { 0u, 0u, 0u, 0u }
        }},
        .MetaDataCount = 1
    };
}

void AssertCompilationErrors(int32_t* utest_result, ShaderCompiler_CompileShader* utest_fixture, const ElemShaderCompilationResult* compilationResult)
{
    auto errorCount = 0u;

    for (uint32_t i = 0; i < compilationResult->Messages.Length; i++)
    {
        if (compilationResult->Messages.Items[i].Type == ElemToolsMessageType_Error)
        {
            //printf("Error: %s\n", compilationResult->Messages.Items[i].Message);
            errorCount++;
        }
    }

    if (!utest_fixture->WithError)
    {
        ASSERT_FALSE_MSG(compilationResult->HasErrors, "Compilation contains errors");
        ASSERT_EQ_MSG(0u, errorCount, "Compilation contains errors");
        ASSERT_LT_MSG(0u, compilationResult->Data.Length, "Compilation data is empty");
    }
    else
    {
        ASSERT_TRUE_MSG(compilationResult->HasErrors, "Compilation contains errors");
        ASSERT_LT_MSG(0u, errorCount, "Compilation contains errors");
        ASSERT_EQ_MSG(0u, compilationResult->Data.Length, "Compilation data is empty");
    }
}

void AssertShaderData(int32_t* utest_result, ShaderCompiler_CompileShader* utest_fixture, const ElemShaderCompilationResult* compilationResult)
{
    if (compilationResult->HasErrors)
    {
        return;
    }

    uint8_t* dataPointer = compilationResult->Data.Items;
   
    char headerSignature[9] = {};

    for (uint32_t i = 0; i < 8; i++)
    {
        headerSignature[i] = *dataPointer;
        dataPointer++;
    }

    ASSERT_STREQ_MSG("ELEMSLIB", headerSignature, "Shader signature is wrong.");

    auto shaderCount = *(uint32_t*)dataPointer;
    dataPointer += sizeof(uint32_t);
    ASSERT_EQ_MSG(utest_fixture->ExpectedShaderCount, shaderCount, "Shaders count is wrong.");

    for (uint32_t i = 0; i < shaderCount; i++)
    {
        auto shaderType = *(ShaderType*)dataPointer;
        dataPointer += sizeof(shaderType);

        auto nameLength = *(uint32_t*)dataPointer;
        dataPointer += sizeof(uint32_t);

        char name[255] = {};

        for (uint32_t j = 0; j < nameLength; j++)
        {
            name[j] = *(char*)dataPointer;
            dataPointer++;
        }

        ShaderInfo* expectedShader = nullptr;

        for (uint32_t j = 0; j < utest_fixture->ExpectedShaderCount; j++)
        {
            if (strstr(utest_fixture->ExpectedShaders[j].Name, name))
            {
                expectedShader = &utest_fixture->ExpectedShaders[j];
                break;
            }
        }

        ASSERT_TRUE_MSG(expectedShader != nullptr, "Cannot find an expected shader.");
        ASSERT_EQ_MSG(expectedShader->Type, shaderType, "Shader Type is wrong.");

        auto shaderMetadataCount = *(uint32_t*)dataPointer;
        dataPointer += sizeof(uint32_t);
        ASSERT_EQ_MSG(expectedShader->MetaDataCount, shaderMetadataCount, "Metadata count is wrong.");

        for (uint32_t j = 0; j < shaderMetadataCount; j++)
        {
            auto expectedMetadata = expectedShader->Metadata[j];

            auto metadataType = *(ShaderMetadataType*)dataPointer;
            dataPointer += sizeof(ShaderMetadataType);

            ASSERT_EQ_MSG(expectedMetadata.Type, metadataType, "Metadata type is wrong.");

            for (uint32_t k = 0; k < 4; k++)
            {
                auto metadataValue = *(uint32_t*)dataPointer;
                dataPointer += sizeof(uint32_t);

                ASSERT_EQ_MSG(expectedMetadata.Value[k], metadataValue, "Metadata value is wrong.");
            }


            auto shaderSizeInBytes = *(uint32_t*)dataPointer;
            dataPointer += sizeof(uint32_t);

            ASSERT_LT_MSG(0u, shaderSizeInBytes, "Shader size should be greater than 0.");
            dataPointer += shaderSizeInBytes;
        }
    }
}

UTEST_F_TEARDOWN(ShaderCompiler_CompileShader) 
{
    // Arrange
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

    // Act
    auto compilationResult = ElemCompileShaderLibrary(utest_fixture->TargetGraphicsApi, utest_fixture->TargetPlatform, &sourceData, nullptr);

    // Assert
    AssertCompilationErrors(utest_result, utest_fixture, &compilationResult);
    AssertShaderData(utest_result, utest_fixture, &compilationResult);
}

UTEST_F(ShaderCompiler_CompileShader, Hlsl_DirectX12_Windows) 
{
    utest_fixture->SourceLanguage = ElemShaderLanguage_Hlsl;
    utest_fixture->TargetGraphicsApi = ElemToolsGraphicsApi_DirectX12;
    utest_fixture->TargetPlatform = ElemToolsPlatform_Windows;
    utest_fixture->WithError = false;
}

UTEST_F(ShaderCompiler_CompileShader, Hlsl_DirectX12_Windows_Error) 
{
    utest_fixture->SourceLanguage = ElemShaderLanguage_Hlsl;
    utest_fixture->TargetGraphicsApi = ElemToolsGraphicsApi_DirectX12;
    utest_fixture->TargetPlatform = ElemToolsPlatform_Windows;
    utest_fixture->WithError = true;
}

UTEST_F(ShaderCompiler_CompileShader, Hlsl_Vulkan_Windows) 
{
    utest_fixture->SourceLanguage = ElemShaderLanguage_Hlsl;
    utest_fixture->TargetGraphicsApi = ElemToolsGraphicsApi_Vulkan;
    utest_fixture->TargetPlatform = ElemToolsPlatform_Windows;
    utest_fixture->WithError = false;
}

UTEST_F(ShaderCompiler_CompileShader, Hlsl_Vulkan_Windows_Error) 
{
    utest_fixture->SourceLanguage = ElemShaderLanguage_Hlsl;
    utest_fixture->TargetGraphicsApi = ElemToolsGraphicsApi_Vulkan;
    utest_fixture->TargetPlatform = ElemToolsPlatform_Windows;
    utest_fixture->WithError = true;
}

UTEST_F(ShaderCompiler_CompileShader, Hlsl_Vulkan_Linux) 
{
    utest_fixture->SourceLanguage = ElemShaderLanguage_Hlsl;
    utest_fixture->TargetGraphicsApi = ElemToolsGraphicsApi_Vulkan;
    utest_fixture->TargetPlatform = ElemToolsPlatform_Linux;
    utest_fixture->WithError = false;
}

UTEST_F(ShaderCompiler_CompileShader, Hlsl_Vulkan_Linux_Error) 
{
    utest_fixture->SourceLanguage = ElemShaderLanguage_Hlsl;
    utest_fixture->TargetGraphicsApi = ElemToolsGraphicsApi_Vulkan;
    utest_fixture->TargetPlatform = ElemToolsPlatform_Linux;
    utest_fixture->WithError = true;
}

#ifndef __linux__
UTEST_F(ShaderCompiler_CompileShader, Hlsl_Metal_MacOS) 
{
    utest_fixture->SourceLanguage = ElemShaderLanguage_Hlsl;
    utest_fixture->TargetGraphicsApi = ElemToolsGraphicsApi_Metal;
    utest_fixture->TargetPlatform = ElemToolsPlatform_MacOS;
    utest_fixture->WithError = false;
}

UTEST_F(ShaderCompiler_CompileShader, Hlsl_Metal_MacOS_Error) 
{
    utest_fixture->SourceLanguage = ElemShaderLanguage_Hlsl;
    utest_fixture->TargetGraphicsApi = ElemToolsGraphicsApi_Metal;
    utest_fixture->TargetPlatform = ElemToolsPlatform_MacOS;
    utest_fixture->WithError = true;
}

UTEST_F(ShaderCompiler_CompileShader, Hlsl_Metal_iOS) 
{
    utest_fixture->SourceLanguage = ElemShaderLanguage_Hlsl;
    utest_fixture->TargetGraphicsApi = ElemToolsGraphicsApi_Metal;
    utest_fixture->TargetPlatform = ElemToolsPlatform_iOS;
    utest_fixture->WithError = false;
}

UTEST_F(ShaderCompiler_CompileShader, Hlsl_Metal_iOS_Error) 
{
    utest_fixture->SourceLanguage = ElemShaderLanguage_Hlsl;
    utest_fixture->TargetGraphicsApi = ElemToolsGraphicsApi_Metal;
    utest_fixture->TargetPlatform = ElemToolsPlatform_iOS;
    utest_fixture->WithError = true;
}
#endif
