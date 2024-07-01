#include "Elemental.h"
#include "GraphicsTests.h"
#include "utest.h"

// TODO: Test Viewports
// TODO: Render on a render target texture
// TODO: Check command list type when dispatch mesh
// TODO: Multiple config for rendering

void TestBeginClearRenderPass(ElemCommandList commandList, ElemGraphicsResourceDescriptor renderTarget, ElemColor clearColor)
{
    ElemRenderPassRenderTarget renderPassRenderTarget = 
    {
        .RenderTarget = renderTarget,
        .ClearColor = clearColor,
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

    // TODO: Barrier to RTV?

    // Act
    ElemBeginRenderPass(commandList, &parameters);
}

UTEST(Rendering, RenderPassClearRenderTarget) 
{
    // Arrange
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);
    auto commandQueue = ElemCreateCommandQueue(graphicsDevice, ElemCommandQueueType_Graphics, nullptr);
    auto commandList = ElemGetCommandList(commandQueue, nullptr);

    auto renderTarget = TestCreateRenderTarget(graphicsDevice, 16, 16, ElemGraphicsFormat_R32G32B32A32_FLOAT);

    // TODO: Barrier to RTV?

    // Act
    TestBeginClearRenderPass(commandList, renderTarget.RenderTargetDescriptor, { .Red = 1.0f, .Green = 0.5f, .Blue = 0.25f, .Alpha = 0.95f });
    ElemEndRenderPass(commandList);
    
    // Assert
    ElemCommitCommandList(commandList);
    auto fence = ElemExecuteCommandList(commandQueue, commandList, nullptr);
    ElemWaitForFenceOnCpu(fence);

    // TODO: Barrier to ShaderRead?

    auto readbackBuffer = TestCreateReadbackBuffer(graphicsDevice, 16 * 16 * 4 * sizeof(float));
    uint32_t resourceIdList[] = { (uint32_t)renderTarget.ReadDescriptor, (uint32_t)readbackBuffer.Descriptor };
    TestDispatchComputeForReadbackBuffer(graphicsDevice, commandQueue, "RenderingTests.shader", "CopyTexture", 1, 1, 1, &resourceIdList);
    auto bufferData = ElemGetGraphicsResourceDataSpan(readbackBuffer.Buffer);

    TestFreeReadbackBuffer(readbackBuffer);
    TestFreeRenderTarget(renderTarget);
    ElemFreeCommandQueue(commandQueue);
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_LOG_NOERROR();

    auto floatData = (float*)bufferData.Items;

    for (uint32_t i = 0; i < bufferData.Length / 4; i += 4)
    {
        ASSERT_EQ_MSG(floatData[i], 1.0f, "Red channel data is invalid.");
        ASSERT_EQ_MSG(floatData[i + 1], 0.5f, "Green channel data is invalid.");
        ASSERT_EQ_MSG(floatData[i + 2], 0.25f, "Blue channel data is invalid.");
        ASSERT_EQ_MSG(floatData[i + 3], 0.95f, "Alpha channel data is invalid.");
    }
}

UTEST(Rendering, DispatchMesh) 
{
    // Arrange
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);
    auto commandQueue = ElemCreateCommandQueue(graphicsDevice, ElemCommandQueueType_Graphics, nullptr);
    auto commandList = ElemGetCommandList(commandQueue, nullptr);

    auto renderTarget = TestCreateRenderTarget(graphicsDevice, 16, 16, ElemGraphicsFormat_R32G32B32A32_FLOAT);

    auto shaderLibrary = TestOpenShader(graphicsDevice, "RenderingTests.shader");

    ElemGraphicsPipelineStateParameters parameters =
    {
        .ShaderLibrary = shaderLibrary,
        .MeshShaderFunction = "MeshShader",
        .PixelShaderFunction = "PixelShader",
        .TextureFormats = { .Items = &renderTarget.Format, .Length = 1 }
    };

    auto meshShaderPipeline = ElemCompileGraphicsPipelineState(graphicsDevice, &parameters);
    ElemFreeShaderLibrary(shaderLibrary);

    // TODO: Barrier to RTV?

    // Act
    TestBeginClearRenderPass(commandList, renderTarget.RenderTargetDescriptor, { .Red = 0.0f, .Green = 0.0f, .Blue = 1.0f, .Alpha = 1.0f });
    ElemBindPipelineState(commandList, meshShaderPipeline);
    ElemDispatchMesh(commandList, 1, 1, 1);
    ElemEndRenderPass(commandList);
    
    // Assert
    ElemCommitCommandList(commandList);
    auto fence = ElemExecuteCommandList(commandQueue, commandList, nullptr);
    ElemWaitForFenceOnCpu(fence);

    // TODO: Barrier to ShaderRead?

    auto readbackBuffer = TestCreateReadbackBuffer(graphicsDevice, 16 * 16 * 4 * sizeof(float));
    uint32_t resourceIdList[] = { (uint32_t)renderTarget.ReadDescriptor, (uint32_t)readbackBuffer.Descriptor };
    TestDispatchComputeForReadbackBuffer(graphicsDevice, commandQueue, "RenderingTests.shader", "CopyTexture", 1, 1, 1, &resourceIdList);
    auto bufferData = ElemGetGraphicsResourceDataSpan(readbackBuffer.Buffer);

    TestFreeReadbackBuffer(readbackBuffer);
    TestFreeRenderTarget(renderTarget);
    ElemFreeCommandQueue(commandQueue);
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_LOG_NOERROR();

    auto floatData = (float*)bufferData.Items;

    for (uint32_t i = 0; i < bufferData.Length / 4; i += 4)
    {
        ASSERT_EQ_MSG(floatData[i], 1.0f, "Red channel data is invalid.");
        ASSERT_EQ_MSG(floatData[i + 1], 1.0f, "Green channel data is invalid.");
        ASSERT_EQ_MSG(floatData[i + 2], 0.0f, "Blue channel data is invalid.");
        ASSERT_EQ_MSG(floatData[i + 3], 1.0f, "Alpha channel data is invalid.");
    }
}
