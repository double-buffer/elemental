#include "Elemental.h"
#include "GraphicsTests.h"
#include "utest.h"

// TODO: Test Viewports
// TODO: Render on a render target texture
// TODO: Check command list type when dispatch mesh
// TODO: Multiple config for rendering

UTEST(Rendering, RenderPassClearRenderTarget) 
{
    // Arrange
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);
    auto commandQueue = ElemCreateCommandQueue(graphicsDevice, ElemCommandQueueType_Graphics, nullptr);
    auto commandList = ElemGetCommandList(commandQueue, nullptr);

    auto renderTextureInfo = ElemCreateTexture2DResourceInfo(graphicsDevice, 16, 16, 1, ElemGraphicsFormat_R32G32B32A32_FLOAT, ElemGraphicsResourceUsage_RenderTarget, nullptr);
    auto graphicsHeap = ElemCreateGraphicsHeap(graphicsDevice, renderTextureInfo.SizeInBytes, nullptr);
    auto renderTexture = ElemCreateGraphicsResource(graphicsHeap, 0, &renderTextureInfo);
    auto renderTextureReadDescriptor = ElemCreateGraphicsResourceDescriptor(renderTexture, ElemGraphicsResourceUsage_Standard, nullptr);
    auto renderTargetDescriptor = ElemCreateGraphicsResourceDescriptor(renderTexture, ElemGraphicsResourceUsage_RenderTarget, nullptr);

    ElemRenderPassRenderTarget renderTarget = 
    {
        .RenderTarget = renderTargetDescriptor,
        .ClearColor = { .Red = 1.0f, .Green = 0.5f, .Blue = 0.25f, .Alpha = 0.95f },
        .LoadAction = ElemRenderPassLoadAction_Clear
    };

    ElemBeginRenderPassParameters parameters =
    {
        .RenderTargets =
        { 
            .Items = &renderTarget,
            .Length = 1
        }
    };

    // TODO: Barrier to RTV?

    // Act
    ElemBeginRenderPass(commandList, &parameters);
    ElemEndRenderPass(commandList);
    
    // Assert
    ElemCommitCommandList(commandList);
    auto fence = ElemExecuteCommandList(commandQueue, commandList, nullptr);
    ElemWaitForFenceOnCpu(fence);

    // TODO: Barrier to ShaderRead?

    auto readbackBuffer = TestCreateReadbackBuffer(graphicsDevice, 16 * 16 * 4 * sizeof(float));
    uint32_t resourceIdList[] = { (uint32_t)renderTextureReadDescriptor, (uint32_t)readbackBuffer.Descriptor };
    TestDispatchComputeForReadbackBuffer(graphicsDevice, commandQueue, "RenderingTests.shader", "CopyTexture", 1, 1, 1, &resourceIdList);
    auto bufferData = ElemGetGraphicsResourceDataSpan(readbackBuffer.Buffer);

    TestFreeReadbackBuffer(readbackBuffer);
    ElemFreeGraphicsResourceDescriptor(renderTargetDescriptor, nullptr);
    ElemFreeGraphicsResource(renderTexture, nullptr);
    ElemFreeGraphicsHeap(graphicsHeap);
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


