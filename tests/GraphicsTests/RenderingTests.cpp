#include "Elemental.h"
#include "GraphicsTests.h"
#include "utest.h"

// TODO: Test Viewports
// TODO: Test Multiple render targets
// TODO: Render on a render target texture
// TODO: Check command list type when dispatch mesh
// TODO: Multiple config for rendering
// TODO: Test Barrier log

UTEST(Rendering, RenderPassClearRenderTarget) 
{
    // Arrange
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);
    auto commandQueue = ElemCreateCommandQueue(graphicsDevice, ElemCommandQueueType_Graphics, nullptr);
    auto commandList = ElemGetCommandList(commandQueue, nullptr);

    auto renderTarget = TestCreateGpuTexture(graphicsDevice, 16, 16, ElemGraphicsFormat_R32G32B32A32_FLOAT, ElemGraphicsResourceUsage_RenderTarget);

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

    auto readbackBuffer = TestCreateGpuBuffer(graphicsDevice, 16 * 16 * 4 * sizeof(float), ElemGraphicsHeapType_Readback);
    uint32_t resourceIdList[] = { (uint32_t)renderTarget.ReadDescriptor, (uint32_t)readbackBuffer.WriteDescriptor };
    TestDispatchComputeForReadbackBuffer(graphicsDevice, commandQueue, "Assert.shader", "CopyTexture", 1, 1, 1, &resourceIdList);
    auto bufferData = ElemGetGraphicsResourceDataSpan(readbackBuffer.Buffer);

    TestFreeGpuBuffer(readbackBuffer);
    TestFreeGpuTexture(renderTarget);
    ElemFreeCommandQueue(commandQueue);
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_LOG_NOERROR();

    auto floatData = (float*)bufferData.Items;

    for (uint32_t i = 0; i < bufferData.Length / sizeof(float); i += 4)
    {
        ASSERT_EQ_MSG(floatData[i], 1.0f, "Red channel data is invalid.");
        ASSERT_EQ_MSG(floatData[i + 1], 0.5f, "Green channel data is invalid.");
        ASSERT_EQ_MSG(floatData[i + 2], 0.25f, "Blue channel data is invalid.");
        ASSERT_EQ_MSG(floatData[i + 3], 0.95f, "Alpha channel data is invalid.");
    }
}

UTEST(Rendering, RenderPassClearDepthBuffer) 
{
    // Arrange
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);
    auto commandQueue = ElemCreateCommandQueue(graphicsDevice, ElemCommandQueueType_Graphics, nullptr);
    auto commandList = ElemGetCommandList(commandQueue, nullptr);

    auto depthBuffer = TestCreateGpuTexture(graphicsDevice, 16, 16, ElemGraphicsFormat_D32_FLOAT, ElemGraphicsResourceUsage_DepthStencil);

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

    auto readbackBuffer = TestCreateGpuBuffer(graphicsDevice, 16 * 16 * sizeof(float), ElemGraphicsHeapType_Readback);
    uint32_t resourceIdList[] = { (uint32_t)depthBuffer.ReadDescriptor, (uint32_t)readbackBuffer.WriteDescriptor };
    TestDispatchComputeForReadbackBuffer(graphicsDevice, commandQueue, "Assert.shader", "CopyTextureFloat", 1, 1, 1, &resourceIdList);
    auto bufferData = ElemGetGraphicsResourceDataSpan(readbackBuffer.Buffer);

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

    auto renderTarget = TestCreateGpuTexture(graphicsDevice, 16, 16, ElemGraphicsFormat_R32G32B32A32_FLOAT, ElemGraphicsResourceUsage_RenderTarget);
    
    ElemGraphicsPipelineStateParameters psoParameters =
    {
        .RenderTargetFormats = { .Items = &renderTarget.Format, .Length = 1 }
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

    auto readbackBuffer = TestCreateGpuBuffer(graphicsDevice, 16 * 16 * 4 * sizeof(float), ElemGraphicsHeapType_Readback);
    uint32_t resourceIdList[] = { (uint32_t)renderTarget.ReadDescriptor, (uint32_t)readbackBuffer.WriteDescriptor };
    TestDispatchComputeForReadbackBuffer(graphicsDevice, commandQueue, "Assert.shader", "CopyTexture", 1, 1, 1, &resourceIdList);
    auto bufferData = ElemGetGraphicsResourceDataSpan(readbackBuffer.Buffer);

    TestFreeGpuBuffer(readbackBuffer);
    TestFreeGpuTexture(renderTarget);
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

// TODO: Test Depth buffer (Clear with a specific value and rendering)
