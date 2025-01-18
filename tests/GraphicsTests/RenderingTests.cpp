#include "Elemental.h"
#include "GraphicsTests.h"
#include "utest.h"

// TODO: Test Multiple render targets
// TODO: Test Multiple Viewports
// TODO: Test Multiple ScissorRectangles
// TODO: Check command list type when dispatch mesh
// TODO: Multiple config for rendering

UTEST(Rendering, RenderPassClearRenderTarget) 
{
    // Arrange
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);
    auto commandQueue = ElemCreateCommandQueue(graphicsDevice, ElemCommandQueueType_Graphics, nullptr);
    auto commandList = ElemGetCommandList(commandQueue, nullptr);

    auto textureSize = 16u;
    auto renderTarget = TestCreateGpuTexture(graphicsDevice, textureSize, textureSize, ElemGraphicsFormat_R32G32B32A32_FLOAT, ElemGraphicsResourceUsage_RenderTarget);

    // Act
    ElemRenderPassRenderTarget renderPassRenderTarget = 
    {
        .RenderTarget = renderTarget.Texture,
        .ClearColor = { .Red = 1.0f, .Green = 0.5f, .Blue = 0.25f, .Alpha = 0.95f },
        .LoadAction = ElemRenderPassLoadAction_Clear
    };

    ElemBeginRenderPassParameters parameters =
    {
        .RenderTargets =
        { 
            .Items = &renderPassRenderTarget,
            .Length = 1
        }
    };

    ElemBeginRenderPass(commandList, &parameters);
    ElemEndRenderPass(commandList);
    
    // Assert
    ElemCommitCommandList(commandList);
    auto fence = ElemExecuteCommandList(commandQueue, commandList, nullptr);
    ElemWaitForFenceOnCpu(fence);

    auto readbackBuffer = TestCreateGpuBuffer(graphicsDevice, textureSize * textureSize * 4 * sizeof(float), ElemGraphicsHeapType_Readback);
    uint32_t resourceIdList[] = { (uint32_t)renderTarget.ReadDescriptor, (uint32_t)readbackBuffer.WriteDescriptor };
    TestDispatchComputeForShader(graphicsDevice, commandQueue, "Assert.shader", "CopyTexture", textureSize / 8, textureSize / 8, 1, &resourceIdList);
    auto bufferData = ElemDownloadGraphicsBufferData(readbackBuffer.Buffer, nullptr);

    TestFreeGpuBuffer(readbackBuffer);
    TestFreeGpuTexture(renderTarget);
    ElemFreeCommandQueue(commandQueue);
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_LOG_NOERROR();
    ASSERT_COLOR_BUFFER(bufferData, 1.0f, 0.5f, 0.25f, 0.95f);
}

UTEST(Rendering, RenderPassClearDepthBuffer) 
{
    // Arrange
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);
    auto commandQueue = ElemCreateCommandQueue(graphicsDevice, ElemCommandQueueType_Graphics, nullptr);
    auto commandList = ElemGetCommandList(commandQueue, nullptr);

    auto textureSize = 16u;
    auto depthBuffer = TestCreateGpuTexture(graphicsDevice, textureSize, textureSize, ElemGraphicsFormat_D32_FLOAT, ElemGraphicsResourceUsage_DepthStencil);

    // Act
    ElemBeginRenderPassParameters parameters =
    {
        .DepthStencil = 
        {
            .DepthStencil = depthBuffer.Texture,
            .DepthClearValue = 0.5f,
            .DepthLoadAction = ElemRenderPassLoadAction_Clear
        }
    };

    ElemBeginRenderPass(commandList, &parameters);
    ElemEndRenderPass(commandList);
    
    // Assert
    ElemCommitCommandList(commandList);
    auto fence = ElemExecuteCommandList(commandQueue, commandList, nullptr);
    ElemWaitForFenceOnCpu(fence);

    auto readbackBuffer = TestCreateGpuBuffer(graphicsDevice, textureSize * textureSize * sizeof(float), ElemGraphicsHeapType_Readback);
    uint32_t resourceIdList[] = { (uint32_t)depthBuffer.ReadDescriptor, (uint32_t)readbackBuffer.WriteDescriptor };
    TestDispatchComputeForShader(graphicsDevice, commandQueue, "Assert.shader", "CopyTextureFloat", textureSize / 8, textureSize / 8, 1, &resourceIdList);
    auto bufferData = ElemDownloadGraphicsBufferData(readbackBuffer.Buffer, nullptr);

    TestFreeGpuBuffer(readbackBuffer);
    TestFreeGpuTexture(depthBuffer);
    ElemFreeCommandQueue(commandQueue);
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_LOG_NOERROR();

    auto floatData = (float*)bufferData.Items;

    for (uint32_t i = 0; i < bufferData.Length / sizeof(float); i++)
    {
        ASSERT_EQ_MSG(floatData[i], 0.5f, "Depth data is invalid.");
    }
}

UTEST(Rendering, DispatchMesh) 
{
    // Arrange
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);
    auto commandQueue = ElemCreateCommandQueue(graphicsDevice, ElemCommandQueueType_Graphics, nullptr);
    auto commandList = ElemGetCommandList(commandQueue, nullptr);

    auto textureSize = 16u;
    auto renderTarget = TestCreateGpuTexture(graphicsDevice, textureSize, textureSize, ElemGraphicsFormat_R32G32B32A32_FLOAT, ElemGraphicsResourceUsage_RenderTarget);

    ElemGraphicsPipelineStateRenderTarget psoRenderTarget = { .Format = renderTarget.Format };
    ElemGraphicsPipelineStateParameters psoParameters =
    {
        .RenderTargets = { .Items = &psoRenderTarget, .Length = 1 }
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

    auto readbackBuffer = TestCreateGpuBuffer(graphicsDevice, textureSize * textureSize * 4 * sizeof(float), ElemGraphicsHeapType_Readback);
    uint32_t resourceIdList[] = { (uint32_t)renderTarget.ReadDescriptor, (uint32_t)readbackBuffer.WriteDescriptor };
    TestDispatchComputeForShader(graphicsDevice, commandQueue, "Assert.shader", "CopyTexture", textureSize / 8, textureSize / 8, 1, &resourceIdList);
    auto bufferData = ElemDownloadGraphicsBufferData(readbackBuffer.Buffer, nullptr);

    TestFreeGpuBuffer(readbackBuffer);
    TestFreeGpuTexture(renderTarget);
    ElemFreePipelineState(meshShaderPipeline);
    ElemFreeCommandQueue(commandQueue);
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_LOG_NOERROR();
    ASSERT_COLOR_BUFFER(bufferData, 1.0f, 1.0f, 0.0f, 1.0f);
}

UTEST(Rendering, BeginRenderPass_SetViewport) 
{
    // Arrange
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);
    auto commandQueue = ElemCreateCommandQueue(graphicsDevice, ElemCommandQueueType_Graphics, nullptr);
    auto commandList = ElemGetCommandList(commandQueue, nullptr);

    auto textureSize = 16u;
    auto renderTarget = TestCreateGpuTexture(graphicsDevice, textureSize, textureSize, ElemGraphicsFormat_R32G32B32A32_FLOAT, ElemGraphicsResourceUsage_RenderTarget);

    ElemGraphicsPipelineStateRenderTarget psoRenderTarget = { .Format = renderTarget.Format };
    ElemGraphicsPipelineStateParameters psoParameters =
    {
        .RenderTargets = { .Items = &psoRenderTarget, .Length = 1 }
    };

    auto meshShaderPipeline = TestOpenMeshShader(graphicsDevice, "RenderingTests.shader", "MeshShader", "PixelShader", &psoParameters);

    // Act
    ElemRenderPassRenderTarget renderPassRenderTarget = 
    {
        .RenderTarget = renderTarget.Texture,
        .ClearColor = { .Red = 0.0f, .Green = 0.0f, .Blue = 1.0f, .Alpha = 1.0f },
        .LoadAction = ElemRenderPassLoadAction_Clear
    };

    ElemViewport renderPassViewport = 
    {
        .X = 2,
        .Y = 2,
        .Width = 4,
        .Height = 4
    };

    ElemBeginRenderPassParameters parameters =
    {
        .RenderTargets =
        { 
            .Items = &renderPassRenderTarget,
            .Length = 1
        },
        .Viewports =
        {
            .Items = &renderPassViewport,
            .Length = 1
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

    auto readbackBuffer = TestCreateGpuBuffer(graphicsDevice, textureSize * textureSize * 4 * sizeof(float), ElemGraphicsHeapType_Readback);
    uint32_t resourceIdList[] = { (uint32_t)renderTarget.ReadDescriptor, (uint32_t)readbackBuffer.WriteDescriptor };
    TestDispatchComputeForShader(graphicsDevice, commandQueue, "Assert.shader", "CopyTexture", textureSize / 8, textureSize / 8, 1, &resourceIdList);
    auto bufferData = ElemDownloadGraphicsBufferData(readbackBuffer.Buffer, nullptr);

    TestFreeGpuBuffer(readbackBuffer);
    TestFreeGpuTexture(renderTarget);
    ElemFreePipelineState(meshShaderPipeline);
    ElemFreeCommandQueue(commandQueue);
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_LOG_NOERROR();
    ASSERT_COLOR_BUFFER_RECTANGLE(bufferData, textureSize, 
                                  2, 2, 4, 4, 
                                  1.0f, 1.0f, 0.0f, 1.0f, 
                                  0.0f, 0.0f, 1.0f, 1.0f);
}

UTEST(Rendering, BeginRenderPass_SetScissorRectangle) 
{
    // Arrange
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);
    auto commandQueue = ElemCreateCommandQueue(graphicsDevice, ElemCommandQueueType_Graphics, nullptr);
    auto commandList = ElemGetCommandList(commandQueue, nullptr);

    auto textureSize = 16u;
    auto renderTarget = TestCreateGpuTexture(graphicsDevice, textureSize, textureSize, ElemGraphicsFormat_R32G32B32A32_FLOAT, ElemGraphicsResourceUsage_RenderTarget);

    ElemGraphicsPipelineStateRenderTarget psoRenderTarget = { .Format = renderTarget.Format };
    ElemGraphicsPipelineStateParameters psoParameters =
    {
        .RenderTargets = { .Items = &psoRenderTarget, .Length = 1 }
    };

    auto meshShaderPipeline = TestOpenMeshShader(graphicsDevice, "RenderingTests.shader", "MeshShader", "PixelShader", &psoParameters);

    // Act
    ElemRenderPassRenderTarget renderPassRenderTarget = 
    {
        .RenderTarget = renderTarget.Texture,
        .ClearColor = { .Red = 0.0f, .Green = 0.0f, .Blue = 1.0f, .Alpha = 1.0f },
        .LoadAction = ElemRenderPassLoadAction_Clear
    };

    ElemRectangle renderPassScissorRectangle = 
    {
        .X = 2,
        .Y = 2,
        .Width = 4,
        .Height = 4
    };

    ElemBeginRenderPassParameters parameters =
    {
        .RenderTargets =
        { 
            .Items = &renderPassRenderTarget,
            .Length = 1
        },
        .ScissorRectangles =
        {
            .Items = &renderPassScissorRectangle,
            .Length = 1
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

    auto readbackBuffer = TestCreateGpuBuffer(graphicsDevice, textureSize * textureSize * 4 * sizeof(float), ElemGraphicsHeapType_Readback);
    uint32_t resourceIdList[] = { (uint32_t)renderTarget.ReadDescriptor, (uint32_t)readbackBuffer.WriteDescriptor };
    TestDispatchComputeForShader(graphicsDevice, commandQueue, "Assert.shader", "CopyTexture", textureSize / 8, textureSize / 8, 1, &resourceIdList);
    auto bufferData = ElemDownloadGraphicsBufferData(readbackBuffer.Buffer, nullptr);

    TestFreeGpuBuffer(readbackBuffer);
    TestFreeGpuTexture(renderTarget);
    ElemFreePipelineState(meshShaderPipeline);
    ElemFreeCommandQueue(commandQueue);
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_LOG_NOERROR();
    ASSERT_COLOR_BUFFER_RECTANGLE(bufferData, textureSize, 
                                  2, 2, 4, 4, 
                                  1.0f, 1.0f, 0.0f, 1.0f, 
                                  0.0f, 0.0f, 1.0f, 1.0f);
}

UTEST(Rendering, SetViewport) 
{
    // Arrange
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);
    auto commandQueue = ElemCreateCommandQueue(graphicsDevice, ElemCommandQueueType_Graphics, nullptr);
    auto commandList = ElemGetCommandList(commandQueue, nullptr);

    auto textureSize = 16u;
    auto renderTarget = TestCreateGpuTexture(graphicsDevice, textureSize, textureSize, ElemGraphicsFormat_R32G32B32A32_FLOAT, ElemGraphicsResourceUsage_RenderTarget);

    ElemGraphicsPipelineStateRenderTarget psoRenderTarget = { .Format = renderTarget.Format };
    ElemGraphicsPipelineStateParameters psoParameters =
    {
        .RenderTargets = { .Items = &psoRenderTarget, .Length = 1 }
    };

    auto meshShaderPipeline = TestOpenMeshShader(graphicsDevice, "RenderingTests.shader", "MeshShader", "PixelShader", &psoParameters);

    // Act
    ElemRenderPassRenderTarget renderPassRenderTarget = 
    {
        .RenderTarget = renderTarget.Texture,
        .ClearColor = { .Red = 0.0f, .Green = 0.0f, .Blue = 1.0f, .Alpha = 1.0f },
        .LoadAction = ElemRenderPassLoadAction_Clear
    };

    ElemViewport viewport = 
    {
        .X = 2,
        .Y = 2,
        .Width = 4,
        .Height = 4
    };

    ElemBeginRenderPassParameters parameters =
    {
        .RenderTargets =
        { 
            .Items = &renderPassRenderTarget,
            .Length = 1
        } 
    };

    ElemBeginRenderPass(commandList, &parameters);
    ElemBindPipelineState(commandList, meshShaderPipeline);
    ElemSetViewport(commandList, &viewport);
    ElemDispatchMesh(commandList, 1, 1, 1);
    ElemEndRenderPass(commandList);
    
    // Assert
    ElemCommitCommandList(commandList);
    auto fence = ElemExecuteCommandList(commandQueue, commandList, nullptr);
    ElemWaitForFenceOnCpu(fence);

    auto readbackBuffer = TestCreateGpuBuffer(graphicsDevice, textureSize * textureSize * 4 * sizeof(float), ElemGraphicsHeapType_Readback);
    uint32_t resourceIdList[] = { (uint32_t)renderTarget.ReadDescriptor, (uint32_t)readbackBuffer.WriteDescriptor };
    TestDispatchComputeForShader(graphicsDevice, commandQueue, "Assert.shader", "CopyTexture", textureSize / 8, textureSize / 8, 1, &resourceIdList);
    auto bufferData = ElemDownloadGraphicsBufferData(readbackBuffer.Buffer, nullptr);

    TestFreeGpuBuffer(readbackBuffer);
    TestFreeGpuTexture(renderTarget);
    ElemFreePipelineState(meshShaderPipeline);
    ElemFreeCommandQueue(commandQueue);
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_LOG_NOERROR();
    ASSERT_COLOR_BUFFER_RECTANGLE(bufferData, textureSize, 
                                  2, 2, 4, 4, 
                                  1.0f, 1.0f, 0.0f, 1.0f, 
                                  0.0f, 0.0f, 1.0f, 1.0f);
}

UTEST(Rendering, SetScissorRectangle) 
{
    // Arrange
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);
    auto commandQueue = ElemCreateCommandQueue(graphicsDevice, ElemCommandQueueType_Graphics, nullptr);
    auto commandList = ElemGetCommandList(commandQueue, nullptr);

    auto textureSize = 16u;
    auto renderTarget = TestCreateGpuTexture(graphicsDevice, textureSize, textureSize, ElemGraphicsFormat_R32G32B32A32_FLOAT, ElemGraphicsResourceUsage_RenderTarget);

    ElemGraphicsPipelineStateRenderTarget psoRenderTarget = { .Format = renderTarget.Format };
    ElemGraphicsPipelineStateParameters psoParameters =
    {
        .RenderTargets = { .Items = &psoRenderTarget, .Length = 1 }
    };

    auto meshShaderPipeline = TestOpenMeshShader(graphicsDevice, "RenderingTests.shader", "MeshShader", "PixelShader", &psoParameters);

    // Act
    ElemRenderPassRenderTarget renderPassRenderTarget = 
    {
        .RenderTarget = renderTarget.Texture,
        .ClearColor = { .Red = 0.0f, .Green = 0.0f, .Blue = 1.0f, .Alpha = 1.0f },
        .LoadAction = ElemRenderPassLoadAction_Clear
    };

    ElemRectangle scissorRectangle = 
    {
        .X = 2,
        .Y = 2,
        .Width = 4,
        .Height = 4
    };

    ElemBeginRenderPassParameters parameters =
    {
        .RenderTargets =
        { 
            .Items = &renderPassRenderTarget,
            .Length = 1
        } 
    };

    ElemBeginRenderPass(commandList, &parameters);
    ElemBindPipelineState(commandList, meshShaderPipeline);
    ElemSetScissorRectangle(commandList, &scissorRectangle);
    ElemDispatchMesh(commandList, 1, 1, 1);
    ElemEndRenderPass(commandList);
    
    // Assert
    ElemCommitCommandList(commandList);
    auto fence = ElemExecuteCommandList(commandQueue, commandList, nullptr);
    ElemWaitForFenceOnCpu(fence);

    auto readbackBuffer = TestCreateGpuBuffer(graphicsDevice, textureSize * textureSize * 4 * sizeof(float), ElemGraphicsHeapType_Readback);
    uint32_t resourceIdList[] = { (uint32_t)renderTarget.ReadDescriptor, (uint32_t)readbackBuffer.WriteDescriptor };
    TestDispatchComputeForShader(graphicsDevice, commandQueue, "Assert.shader", "CopyTexture", textureSize / 8, textureSize / 8, 1, &resourceIdList);
    auto bufferData = ElemDownloadGraphicsBufferData(readbackBuffer.Buffer, nullptr);

    TestFreeGpuBuffer(readbackBuffer);
    TestFreeGpuTexture(renderTarget);
    ElemFreePipelineState(meshShaderPipeline);
    ElemFreeCommandQueue(commandQueue);
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_LOG_NOERROR();
    ASSERT_COLOR_BUFFER_RECTANGLE(bufferData, textureSize, 
                                  2, 2, 4, 4, 
                                  1.0f, 1.0f, 0.0f, 1.0f, 
                                  0.0f, 0.0f, 1.0f, 1.0f);
}
