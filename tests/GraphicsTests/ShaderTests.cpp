#include "Elemental.h"
#include "GraphicsTests.h"
#include "utest.h"

// TODO: Validate dispatch thread group count
// TODO: Cannot push constant before binding pso
// TODO: PSO Multi render targets
// TODO: PSO Blend state
// TODO: PSO Cull order


UTEST(Shader, CompileComputePipelineState) 
{
    // Arrange
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);
    auto shaderLibrary = TestOpenShader(graphicsDevice, "ShaderTests.shader");
    ASSERT_NE(ELEM_HANDLE_NULL, shaderLibrary);

    // Act
    ElemComputePipelineStateParameters parameters =
    {
        .ShaderLibrary = shaderLibrary,
        .ComputeShaderFunction = "TestCompute"
    };

    auto pipelineState = ElemCompileComputePipelineState(graphicsDevice, &parameters); 

    // Assert
    ElemFreeShaderLibrary(shaderLibrary);
    ElemFreePipelineState(pipelineState);
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_LOG_NOERROR();
    ASSERT_NE(ELEM_HANDLE_NULL, pipelineState);
}

UTEST(Shader, CompileComputePipelineStateFunctionNotExist) 
{
    // Arrange
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);
    auto shaderLibrary = TestOpenShader(graphicsDevice, "ShaderTests.shader");
    ASSERT_NE(shaderLibrary, ELEM_HANDLE_NULL);

    // Act
    ElemComputePipelineStateParameters parameters =
    {
        .ShaderLibrary = shaderLibrary,
        .ComputeShaderFunction = "TestComputeNotExist"
    };

    auto pipelineState = ElemCompileComputePipelineState(graphicsDevice, &parameters); 

    // Assert
    ElemFreeShaderLibrary(shaderLibrary);
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_LOG_MESSAGE("Cannot find shader function");
    ASSERT_EQ(ELEM_HANDLE_NULL, pipelineState);
}

UTEST(Shader, DispatchComputeWithoutBindPipelineState) 
{
    // Arrange
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);
    auto commandQueue = ElemCreateCommandQueue(graphicsDevice, ElemCommandQueueType_Graphics, nullptr);
    auto commandList = ElemGetCommandList(commandQueue, nullptr);

    // Act
    ElemDispatchCompute(commandList, 1, 0, 0);

    // Assert
    ElemCommitCommandList(commandList);

    ElemFreeCommandQueue(commandQueue);
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_TRUE_MSG(testHasLogErrors, "Validation logs should have errors.");
    ASSERT_LOG_MESSAGE("A compute pipelinestate must be bound to the commandlist before calling a compute command.");
}

UTEST(Shader, DispatchCompute) 
{
    // Arrange
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);
    auto commandQueue = ElemCreateCommandQueue(graphicsDevice, ElemCommandQueueType_Graphics, nullptr);
    auto readbackBuffer = TestCreateGpuBuffer(graphicsDevice, 64 * sizeof(uint32_t), ElemGraphicsHeapType_Readback);

    // Act
    TestDispatchComputeForReadbackBuffer(graphicsDevice, commandQueue, "ShaderTests.shader", "TestCompute", 1, 1, 1, &readbackBuffer.WriteDescriptor);

    // Assert
    auto bufferData = ElemGetGraphicsResourceDataSpan(readbackBuffer.Buffer);

    TestFreeGpuBuffer(readbackBuffer);
    ElemFreeCommandQueue(commandQueue);
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_LOG_NOERROR();
    auto uintData = (uint32_t*)bufferData.Items;

    for (uint32_t i = 0; i < bufferData.Length / 4; i++)
    {
        ASSERT_EQ_MSG(uintData[i], i < 16 ? i : 0u, "Compute shader data is invalid.");
    }
}

struct Shader_CompileGraphicsPipelineStateDepthCompare
{
    float ClearDepthValue;
    ElemGraphicsCompareFunction CompareFunction;
    float Color[3];
};

UTEST_F_SETUP(Shader_CompileGraphicsPipelineStateDepthCompare) 
{
}

UTEST_F_TEARDOWN(Shader_CompileGraphicsPipelineStateDepthCompare) 
{
    // Arrange
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);
    auto commandQueue = ElemCreateCommandQueue(graphicsDevice, ElemCommandQueueType_Graphics, nullptr);
    auto commandList = ElemGetCommandList(commandQueue, nullptr);

    auto renderTarget = TestCreateGpuTexture(graphicsDevice, 16, 16, ElemGraphicsFormat_R32G32B32A32_FLOAT, ElemGraphicsResourceUsage_RenderTarget);
    auto depthBuffer = TestCreateGpuTexture(graphicsDevice, 16, 16, ElemGraphicsFormat_D32_FLOAT, ElemGraphicsResourceUsage_DepthStencil);
    
    ElemGraphicsPipelineStateParameters psoParameters =
    {
        .RenderTargetFormats = { .Items = &renderTarget.Format, .Length = 1 },
        .DepthStencilFormat = depthBuffer.Format,
        .DepthWrite = true,
        .DepthCompareFunction = utest_fixture->CompareFunction
    };

    auto meshShaderPipeline = TestOpenMeshShader(graphicsDevice, "ShaderTests.shader", "MeshShader", "PixelShader", &psoParameters);

    // Act
    ElemRenderPassRenderTarget renderPassRenderTarget = 
    {
        .RenderTarget = renderTarget.Texture,
        .ClearColor = { .Red = 0.0f, .Green = 0.0f, .Blue = 0.0f, .Alpha = 1.0f },
        .LoadAction = ElemRenderPassLoadAction_Clear
    };

    ElemBeginRenderPassParameters parameters =
    {
        .RenderTargets =
        { 
            .Items = &renderPassRenderTarget,
            .Length = 1
        },
        .DepthStencil = 
        {
            .DepthStencil = depthBuffer.Texture,
            .DepthClearValue = utest_fixture->ClearDepthValue
        }
    };

    ElemBeginRenderPass(commandList, &parameters);
    ElemBindPipelineState(commandList, meshShaderPipeline);

    float shaderParameters[] = { 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.25f };
    ElemPushPipelineStateConstants(commandList, 0, { .Items = (uint8_t*)shaderParameters, .Length = sizeof(float) * 8 });
    ElemDispatchMesh(commandList, 1, 1, 1);

    float shaderParameters2[] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.5f };
    ElemPushPipelineStateConstants(commandList, 0, { .Items = (uint8_t*)shaderParameters2, .Length = sizeof(float) * 8 });
    ElemDispatchMesh(commandList, 1, 1, 1);

    float shaderParameters3[] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.75f };
    ElemPushPipelineStateConstants(commandList, 0, { .Items = (uint8_t*)shaderParameters3, .Length = sizeof(float) * 8 });
    ElemDispatchMesh(commandList, 1, 1, 1);

    ElemEndRenderPass(commandList);
    
    // Assert
    ElemCommitCommandList(commandList);
    auto fence = ElemExecuteCommandList(commandQueue, commandList, nullptr);
    ElemWaitForFenceOnCpu(fence);

    auto readbackBuffer = TestCreateGpuBuffer(graphicsDevice, 16 * 16 * 4 * sizeof(float), ElemGraphicsHeapType_Readback);
    uint32_t resourceIdList[] = { (uint32_t)renderTarget.ReadDescriptor, (uint32_t)readbackBuffer.WriteDescriptor };
    TestDispatchComputeForReadbackBuffer(graphicsDevice, commandQueue, "Assert.shader", "CopyTexture", 1, 1, 1, &resourceIdList);
    auto bufferData = ElemGetGraphicsResourceDataSpan(readbackBuffer.Buffer);

    TestFreeGpuBuffer(readbackBuffer);
    TestFreeGpuTexture(renderTarget);
    TestFreeGpuTexture(depthBuffer);
    ElemFreePipelineState(meshShaderPipeline);
    ElemFreeCommandQueue(commandQueue);
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_LOG_NOERROR();

    auto floatData = (float*)bufferData.Items;

    for (uint32_t i = 0; i < bufferData.Length / sizeof(float); i += 4)
    {
        ASSERT_EQ_MSG(floatData[i], utest_fixture->Color[0], "Red channel data is invalid.");
        ASSERT_EQ_MSG(floatData[i + 1], utest_fixture->Color[1], "Green channel data is invalid.");
        ASSERT_EQ_MSG(floatData[i + 2], utest_fixture->Color[2], "Blue channel data is invalid.");
        ASSERT_EQ_MSG(floatData[i + 3], 1.0f, "Alpha channel data is invalid.");
    }
}

UTEST_F(Shader_CompileGraphicsPipelineStateDepthCompare, Never) 
{
    utest_fixture->ClearDepthValue = 0.5f;
    utest_fixture->CompareFunction = ElemGraphicsCompareFunction_Never;
    utest_fixture->Color[0] = 0.0f;
    utest_fixture->Color[1] = 0.0f;
    utest_fixture->Color[2] = 0.0f;
}

UTEST_F(Shader_CompileGraphicsPipelineStateDepthCompare, Less) 
{
    utest_fixture->ClearDepthValue = 1.0f;
    utest_fixture->CompareFunction = ElemGraphicsCompareFunction_Less;
    utest_fixture->Color[0] = 1.0f;
    utest_fixture->Color[1] = 0.0f;
    utest_fixture->Color[2] = 0.0f;
}

UTEST_F(Shader_CompileGraphicsPipelineStateDepthCompare, LessEqual) 
{
    utest_fixture->ClearDepthValue = 0.25f;
    utest_fixture->CompareFunction = ElemGraphicsCompareFunction_LessEqual;
    utest_fixture->Color[0] = 1.0f;
    utest_fixture->Color[1] = 0.0f;
    utest_fixture->Color[2] = 0.0f;
}

UTEST_F(Shader_CompileGraphicsPipelineStateDepthCompare, Greater) 
{
    utest_fixture->ClearDepthValue = 0.0f;
    utest_fixture->CompareFunction = ElemGraphicsCompareFunction_Greater;
    utest_fixture->Color[0] = 0.0f;
    utest_fixture->Color[1] = 0.0f;
    utest_fixture->Color[2] = 1.0f;
}

UTEST_F(Shader_CompileGraphicsPipelineStateDepthCompare, GreaterEqual) 
{
    utest_fixture->ClearDepthValue = 0.75f;
    utest_fixture->CompareFunction = ElemGraphicsCompareFunction_GreaterEqual;
    utest_fixture->Color[0] = 0.0f;
    utest_fixture->Color[1] = 0.0f;
    utest_fixture->Color[2] = 1.0f;
}

UTEST_F(Shader_CompileGraphicsPipelineStateDepthCompare, Equal) 
{
    utest_fixture->ClearDepthValue = 0.5f;
    utest_fixture->CompareFunction = ElemGraphicsCompareFunction_Equal;
    utest_fixture->Color[0] = 0.0f;
    utest_fixture->Color[1] = 1.0f;
    utest_fixture->Color[2] = 0.0f;
}

UTEST_F(Shader_CompileGraphicsPipelineStateDepthCompare, Always) 
{
    utest_fixture->ClearDepthValue = 1.0f;
    utest_fixture->CompareFunction = ElemGraphicsCompareFunction_Always;
    utest_fixture->Color[0] = 0.0f;
    utest_fixture->Color[1] = 0.0f;
    utest_fixture->Color[2] = 1.0f;
}
// TODO: Check depth stencil format if comparaison function set
