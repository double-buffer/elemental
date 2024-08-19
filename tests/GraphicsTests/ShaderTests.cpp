#include "Elemental.h"
#include "GraphicsTests.h"
#include "utest.h"

// TODO: Validate dispatch thread group count
// TODO: Cannot push constant before binding pso

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

UTEST(Shader, CompileGraphicsPipelineStateDepthCompareGreater) 
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
        .DepthStencilFormat = depthBuffer.Format
    };

    auto meshShaderPipeline = TestOpenMeshShader(graphicsDevice, "RenderingTests.shader", "MeshShader", "PixelShader", &psoParameters);

    // Act
    ElemRenderPassRenderTarget renderPassRenderTarget = 
    {
        .RenderTarget = renderTarget.Texture,
        .ClearColor = { .Red = 0.0f, .Green = 0.0f, .Blue = 1.0f, .Alpha = 1.0f },
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
            .DepthClearValue = 0.0f,
            .DepthLoadAction = ElemRenderPassLoadAction_Clear
        }
    };

    ElemBeginRenderPass(commandList, &parameters);
    ElemBindPipelineState(commandList, meshShaderPipeline);
    ElemDispatchMesh(commandList, 1, 1, 1);
    ElemEndRenderPass(commandList);
    
    // Assert
    ElemCommitCommandList(commandList);
    auto fence = ElemExecuteCommandList(commandQueue, commandList, nullptr);
    ElemWaitForFenceOnCpu(fence);

    auto readbackBuffer = TestCreateGpuBuffer(graphicsDevice, 16 * 16 * 4 * sizeof(float), ElemGraphicsHeapType_Readback);
    uint32_t resourceIdList[] = { (uint32_t)renderTarget.ReadDescriptor, (uint32_t)readbackBuffer.WriteDescriptor };
    TestDispatchComputeForReadbackBuffer(graphicsDevice, commandQueue, "RenderingTests.shader", "CopyTexture", 1, 1, 1, &resourceIdList);
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
        ASSERT_EQ_MSG(floatData[i], 0.0f, "Red channel data is invalid.");
        ASSERT_EQ_MSG(floatData[i + 1], 0.0f, "Green channel data is invalid.");
        ASSERT_EQ_MSG(floatData[i + 2], 1.0f, "Blue channel data is invalid.");
        ASSERT_EQ_MSG(floatData[i + 3], 1.0f, "Alpha channel data is invalid.");
    }
}

UTEST(Shader, CompileGraphicsPipelineStateDepthCompareGreaterOrEquals) 
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
        .DepthStencilFormat = depthBuffer.Format
    };

    auto meshShaderPipeline = TestOpenMeshShader(graphicsDevice, "RenderingTests.shader", "MeshShader", "PixelShader", &psoParameters);

    // Act
    ElemRenderPassRenderTarget renderPassRenderTarget = 
    {
        .RenderTarget = renderTarget.Texture,
        .ClearColor = { .Red = 0.0f, .Green = 0.0f, .Blue = 1.0f, .Alpha = 1.0f },
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
            .DepthClearValue = 0.0f,
            .DepthLoadAction = ElemRenderPassLoadAction_Clear
        }
    };

    ElemBeginRenderPass(commandList, &parameters);
    ElemBindPipelineState(commandList, meshShaderPipeline);
    ElemDispatchMesh(commandList, 1, 1, 1);
    ElemEndRenderPass(commandList);
    
    // Assert
    ElemCommitCommandList(commandList);
    auto fence = ElemExecuteCommandList(commandQueue, commandList, nullptr);
    ElemWaitForFenceOnCpu(fence);

    auto readbackBuffer = TestCreateGpuBuffer(graphicsDevice, 16 * 16 * 4 * sizeof(float), ElemGraphicsHeapType_Readback);
    uint32_t resourceIdList[] = { (uint32_t)renderTarget.ReadDescriptor, (uint32_t)readbackBuffer.WriteDescriptor };
    TestDispatchComputeForReadbackBuffer(graphicsDevice, commandQueue, "RenderingTests.shader", "CopyTexture", 1, 1, 1, &resourceIdList);
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
        ASSERT_EQ_MSG(floatData[i], 1.0f, "Red channel data is invalid.");
        ASSERT_EQ_MSG(floatData[i + 1], 1.0f, "Green channel data is invalid.");
        ASSERT_EQ_MSG(floatData[i + 2], 0.0f, "Blue channel data is invalid.");
        ASSERT_EQ_MSG(floatData[i + 3], 1.0f, "Alpha channel data is invalid.");
    }
}
