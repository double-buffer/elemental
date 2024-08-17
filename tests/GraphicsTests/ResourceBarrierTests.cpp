#include "Elemental.h"
#include "GraphicsTests.h"
#include "utest.h"

// TODO: Test duplicate barriers on resource
// TODO: Multiple rendertargets in begin render
// TODO: For the moment it is impossible to test for present
// TODO: Test Additional commands (when adding new ones)

UTEST(ResourceBarrier, GraphicsResourceBarrier_BufferReadAfterWrite) 
{
    // Arrange
    int32_t elementCount = 1000000;
    uint32_t threadSize = 16;
    uint32_t dispatchX = (elementCount + (threadSize - 1)) / threadSize;
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);
    auto commandQueue = ElemCreateCommandQueue(graphicsDevice, ElemCommandQueueType_Graphics, nullptr);

    auto gpuBuffer = TestCreateGpuBuffer(graphicsDevice, elementCount * sizeof(uint32_t));
    auto readbackBuffer = TestCreateGpuBuffer(graphicsDevice, elementCount * sizeof(uint32_t), ElemGraphicsHeapType_Readback);

    auto writeBufferDataPipelineState = TestOpenComputeShader(graphicsDevice, "ResourceBarrierTests.shader", "TestWriteBufferData");
    auto readBufferDataPipelineState = TestOpenComputeShader(graphicsDevice, "ResourceBarrierTests.shader", "TestReadBufferData");

    // Act
    auto commandList = ElemGetCommandList(commandQueue, nullptr);

    ElemGraphicsResourceBarrier(commandList, gpuBuffer.WriteDescriptor, nullptr);
    TestDispatchCompute(commandList, writeBufferDataPipelineState, dispatchX, 1, 1, { gpuBuffer.WriteDescriptor, 0, elementCount });

    INIT_ASSERT_BARRIER(dispatch1, BARRIER_ARRAY(
        BUFFER_BARRIER(gpuBuffer.Buffer, 
                       ElemGraphicsResourceBarrierSyncType_None, ElemGraphicsResourceBarrierSyncType_Compute, 
                       ElemGraphicsResourceBarrierAccessType_NoAccess, ElemGraphicsResourceBarrierAccessType_Write)
    ), BARRIER_ARRAY_EMPTY());

    ElemGraphicsResourceBarrier(commandList, gpuBuffer.ReadDescriptor, nullptr);
    TestDispatchCompute(commandList, readBufferDataPipelineState, dispatchX, 1, 1, { gpuBuffer.ReadDescriptor, readbackBuffer.WriteDescriptor, elementCount });

    INIT_ASSERT_BARRIER(dispatch2, BARRIER_ARRAY(
        BUFFER_BARRIER(gpuBuffer.Buffer, 
                       ElemGraphicsResourceBarrierSyncType_Compute, ElemGraphicsResourceBarrierSyncType_Compute, 
                       ElemGraphicsResourceBarrierAccessType_Write, ElemGraphicsResourceBarrierAccessType_Read)
    ), BARRIER_ARRAY_EMPTY());

    ElemCommitCommandList(commandList);
    auto fence = ElemExecuteCommandList(commandQueue, commandList, nullptr);

    // Assert
    ElemWaitForFenceOnCpu(fence);
    auto bufferData = ElemGetGraphicsResourceDataSpan(readbackBuffer.Buffer);

    ElemFreePipelineState(readBufferDataPipelineState);
    ElemFreePipelineState(writeBufferDataPipelineState);
    TestFreeGpuBuffer(gpuBuffer);
    TestFreeGpuBuffer(readbackBuffer);
    ElemFreeCommandQueue(commandQueue);
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_LOG_NOERROR();
    ASSERT_BARRIER(dispatch1);
    ASSERT_BARRIER(dispatch2);

    auto intData = (int32_t*)bufferData.Items;

    for (int32_t i = 0; i < elementCount; i++)
    {
        ASSERT_EQ_MSG(intData[i], elementCount - i - 1, "Compute shader data is invalid.");
    }
}

UTEST(ResourceBarrier, GraphicsResourceBarrier_BufferWriteAfterWrite) 
{
    // Arrange
    int32_t elementCount = 1000000;
    uint32_t threadSize = 16;
    uint32_t dispatchX = (elementCount + (threadSize - 1)) / threadSize;
    int32_t addOffset = 28;

    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);
    auto commandQueue = ElemCreateCommandQueue(graphicsDevice, ElemCommandQueueType_Graphics, nullptr);

    auto gpuBuffer = TestCreateGpuBuffer(graphicsDevice, elementCount * sizeof(uint32_t));
    auto readbackBuffer = TestCreateGpuBuffer(graphicsDevice, elementCount * sizeof(uint32_t), ElemGraphicsHeapType_Readback);

    auto writeBufferDataPipelineState = TestOpenComputeShader(graphicsDevice, "ResourceBarrierTests.shader", "TestWriteBufferData");
    auto writeAddBufferDataPipelineState = TestOpenComputeShader(graphicsDevice, "ResourceBarrierTests.shader", "TestWriteAddBufferData");
    auto readBufferDataPipelineState = TestOpenComputeShader(graphicsDevice, "ResourceBarrierTests.shader", "TestReadBufferData");

    // Act
    auto commandList = ElemGetCommandList(commandQueue, nullptr);

    ElemGraphicsResourceBarrier(commandList, gpuBuffer.WriteDescriptor, nullptr);
    TestDispatchCompute(commandList, writeBufferDataPipelineState, dispatchX, 1, 1, { gpuBuffer.WriteDescriptor, 0, elementCount });

    INIT_ASSERT_BARRIER(dispatch1, BARRIER_ARRAY(
        BUFFER_BARRIER(gpuBuffer.Buffer, 
                       ElemGraphicsResourceBarrierSyncType_None, ElemGraphicsResourceBarrierSyncType_Compute, 
                       ElemGraphicsResourceBarrierAccessType_NoAccess, ElemGraphicsResourceBarrierAccessType_Write)
    ), BARRIER_ARRAY_EMPTY());
   
    ElemGraphicsResourceBarrier(commandList, gpuBuffer.WriteDescriptor, nullptr);
    TestDispatchCompute(commandList, writeAddBufferDataPipelineState, dispatchX, 1, 1, { gpuBuffer.WriteDescriptor, addOffset, elementCount });

    INIT_ASSERT_BARRIER(dispatch2, BARRIER_ARRAY(
        BUFFER_BARRIER(gpuBuffer.Buffer, 
                       ElemGraphicsResourceBarrierSyncType_Compute, ElemGraphicsResourceBarrierSyncType_Compute, 
                       ElemGraphicsResourceBarrierAccessType_Write, ElemGraphicsResourceBarrierAccessType_Write)
    ), BARRIER_ARRAY_EMPTY());

    ElemGraphicsResourceBarrier(commandList, gpuBuffer.ReadDescriptor, nullptr);
    TestDispatchCompute(commandList, readBufferDataPipelineState, dispatchX, 1, 1, { gpuBuffer.ReadDescriptor, readbackBuffer.WriteDescriptor, elementCount });

    INIT_ASSERT_BARRIER(dispatch3, BARRIER_ARRAY(
        BUFFER_BARRIER(gpuBuffer.Buffer, 
                       ElemGraphicsResourceBarrierSyncType_Compute, ElemGraphicsResourceBarrierSyncType_Compute, 
                       ElemGraphicsResourceBarrierAccessType_Write, ElemGraphicsResourceBarrierAccessType_Read)
    ), BARRIER_ARRAY_EMPTY());

    ElemCommitCommandList(commandList);
    auto fence = ElemExecuteCommandList(commandQueue, commandList, nullptr);

    // Assert
    ElemWaitForFenceOnCpu(fence);
    auto bufferData = ElemGetGraphicsResourceDataSpan(readbackBuffer.Buffer);

    ElemFreePipelineState(readBufferDataPipelineState);
    ElemFreePipelineState(writeAddBufferDataPipelineState);
    ElemFreePipelineState(writeBufferDataPipelineState);
    TestFreeGpuBuffer(gpuBuffer);
    TestFreeGpuBuffer(readbackBuffer);
    ElemFreeCommandQueue(commandQueue);
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_LOG_NOERROR();
    ASSERT_BARRIER(dispatch1);
    ASSERT_BARRIER(dispatch2);
    ASSERT_BARRIER(dispatch3);

    auto intData = (int32_t*)bufferData.Items;

    for (int32_t i = 0; i < elementCount; i++)
    {
        ASSERT_EQ_MSG(intData[i], elementCount - i - 1 + addOffset, "Compute shader data is invalid.");
    }
}

UTEST(ResourceBarrier, GraphicsResourceBarrier_DifferentBuffers) 
{
    // Arrange
    int32_t elementCount = 1000000;
    uint32_t threadSize = 16;
    uint32_t dispatchX = (elementCount + (threadSize - 1)) / threadSize;

    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);
    auto commandQueue = ElemCreateCommandQueue(graphicsDevice, ElemCommandQueueType_Graphics, nullptr);

    auto gpuBuffer = TestCreateGpuBuffer(graphicsDevice, elementCount * sizeof(uint32_t));
    auto gpuBuffer2 = TestCreateGpuBuffer(graphicsDevice, elementCount * sizeof(uint32_t));
    auto readbackBuffer = TestCreateGpuBuffer(graphicsDevice, elementCount * sizeof(uint32_t), ElemGraphicsHeapType_Readback);

    auto writeBufferDataPipelineState = TestOpenComputeShader(graphicsDevice, "ResourceBarrierTests.shader", "TestWriteBufferData");
    auto readBufferDataPipelineState = TestOpenComputeShader(graphicsDevice, "ResourceBarrierTests.shader", "TestReadBufferData");

    // Act
    auto commandList = ElemGetCommandList(commandQueue, nullptr);

    ElemGraphicsResourceBarrier(commandList, gpuBuffer.WriteDescriptor, nullptr);
    TestDispatchCompute(commandList, writeBufferDataPipelineState, dispatchX, 1, 1, { gpuBuffer.WriteDescriptor, 0, elementCount });

    INIT_ASSERT_BARRIER(dispatch1, BARRIER_ARRAY(
        BUFFER_BARRIER(gpuBuffer.Buffer, 
                       ElemGraphicsResourceBarrierSyncType_None, ElemGraphicsResourceBarrierSyncType_Compute, 
                       ElemGraphicsResourceBarrierAccessType_NoAccess, ElemGraphicsResourceBarrierAccessType_Write)
    ), BARRIER_ARRAY_EMPTY());
   
    ElemGraphicsResourceBarrier(commandList, gpuBuffer.ReadDescriptor, nullptr);
    ElemGraphicsResourceBarrier(commandList, gpuBuffer2.WriteDescriptor, nullptr);
    TestDispatchCompute(commandList, readBufferDataPipelineState, dispatchX, 1, 1, { gpuBuffer.ReadDescriptor, gpuBuffer2.WriteDescriptor, elementCount });

    INIT_ASSERT_BARRIER(dispatch2, BARRIER_ARRAY(
        BUFFER_BARRIER(gpuBuffer.Buffer, 
                       ElemGraphicsResourceBarrierSyncType_Compute, ElemGraphicsResourceBarrierSyncType_Compute, 
                       ElemGraphicsResourceBarrierAccessType_Write, ElemGraphicsResourceBarrierAccessType_Read),
        BUFFER_BARRIER(gpuBuffer2.Buffer, 
                       ElemGraphicsResourceBarrierSyncType_None, ElemGraphicsResourceBarrierSyncType_Compute, 
                       ElemGraphicsResourceBarrierAccessType_NoAccess, ElemGraphicsResourceBarrierAccessType_Write)
    ), BARRIER_ARRAY_EMPTY());

    ElemGraphicsResourceBarrier(commandList, gpuBuffer2.ReadDescriptor, nullptr);
    TestDispatchCompute(commandList, readBufferDataPipelineState, dispatchX, 1, 1, { gpuBuffer2.ReadDescriptor, readbackBuffer.WriteDescriptor, elementCount });

    INIT_ASSERT_BARRIER(dispatch3, BARRIER_ARRAY(
        BUFFER_BARRIER(gpuBuffer2.Buffer, 
                       ElemGraphicsResourceBarrierSyncType_Compute, ElemGraphicsResourceBarrierSyncType_Compute, 
                       ElemGraphicsResourceBarrierAccessType_Write, ElemGraphicsResourceBarrierAccessType_Read)
    ), BARRIER_ARRAY_EMPTY());

    ElemCommitCommandList(commandList);
    auto fence = ElemExecuteCommandList(commandQueue, commandList, nullptr);

    // Assert
    ElemWaitForFenceOnCpu(fence);
    auto bufferData = ElemGetGraphicsResourceDataSpan(readbackBuffer.Buffer);

    ElemFreePipelineState(readBufferDataPipelineState);
    ElemFreePipelineState(writeBufferDataPipelineState);
    TestFreeGpuBuffer(gpuBuffer);
    TestFreeGpuBuffer(gpuBuffer2);
    TestFreeGpuBuffer(readbackBuffer);
    ElemFreeCommandQueue(commandQueue);
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_LOG_NOERROR();
    ASSERT_BARRIER(dispatch1);
    ASSERT_BARRIER(dispatch2);
    ASSERT_BARRIER(dispatch3);

    auto intData = (int32_t*)bufferData.Items;

    for (int32_t i = 0; i < elementCount; i++)
    {
        ASSERT_EQ_MSG(intData[i], i, "Compute shader data is invalid.");
    }
}

UTEST(ResourceBarrier, GraphicsResourceBarrier_TextureReadAfterWrite) 
{
    // Arrange
    uint32_t width = 16;
    uint32_t height = 16;
    uint32_t threadSize = 16;
    uint32_t dispatchX = (width + (threadSize - 1)) / threadSize;
    uint32_t dispatchY = (height + (threadSize - 1)) / threadSize;
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);
    auto commandQueue = ElemCreateCommandQueue(graphicsDevice, ElemCommandQueueType_Graphics, nullptr);

    auto gpuTexture = TestCreateGpuTexture(graphicsDevice, width, height, ElemGraphicsFormat_R16G16B16A16_FLOAT, (ElemGraphicsResourceUsage)(ElemGraphicsResourceUsage_RenderTarget | ElemGraphicsResourceUsage_Write));
    auto readbackBuffer = TestCreateGpuBuffer(graphicsDevice, width * height * 4 * sizeof(float), ElemGraphicsHeapType_Readback);

    auto writeTextureDataPipelineState = TestOpenComputeShader(graphicsDevice, "ResourceBarrierTests.shader", "TestWriteTextureData");
    auto readTextureDataPipelineState = TestOpenComputeShader(graphicsDevice, "ResourceBarrierTests.shader", "TestReadTextureData");

    // Act
    auto commandList = ElemGetCommandList(commandQueue, nullptr);

    ElemGraphicsResourceBarrier(commandList, gpuTexture.WriteDescriptor, nullptr);
    TestDispatchCompute(commandList, writeTextureDataPipelineState, dispatchX, dispatchY, 1, { gpuTexture.WriteDescriptor, 0, 0 });

    INIT_ASSERT_BARRIER(dispatch1, BARRIER_ARRAY_EMPTY(), BARRIER_ARRAY(
        TEXTURE_BARRIER(gpuTexture.Texture, 
                        ElemGraphicsResourceBarrierSyncType_None, ElemGraphicsResourceBarrierSyncType_Compute, 
                        ElemGraphicsResourceBarrierAccessType_NoAccess, ElemGraphicsResourceBarrierAccessType_Write, 
                        ElemGraphicsResourceBarrierLayoutType_Undefined, ElemGraphicsResourceBarrierLayoutType_Write)
    ));

    ElemGraphicsResourceBarrier(commandList, gpuTexture.ReadDescriptor, nullptr);
    TestDispatchCompute(commandList, readTextureDataPipelineState, dispatchX, dispatchY, 1, { gpuTexture.ReadDescriptor, readbackBuffer.WriteDescriptor, 0 });

    INIT_ASSERT_BARRIER(dispatch2, BARRIER_ARRAY_EMPTY(), BARRIER_ARRAY(
        TEXTURE_BARRIER(gpuTexture.Texture, 
                        ElemGraphicsResourceBarrierSyncType_Compute, ElemGraphicsResourceBarrierSyncType_Compute, 
                        ElemGraphicsResourceBarrierAccessType_Write, ElemGraphicsResourceBarrierAccessType_Read, 
                        ElemGraphicsResourceBarrierLayoutType_Write, ElemGraphicsResourceBarrierLayoutType_Read)
    ));

    ElemCommitCommandList(commandList);
    auto fence = ElemExecuteCommandList(commandQueue, commandList, nullptr);

    // Assert
    ElemWaitForFenceOnCpu(fence);
    auto bufferData = ElemGetGraphicsResourceDataSpan(readbackBuffer.Buffer);

    ElemFreePipelineState(readTextureDataPipelineState);
    ElemFreePipelineState(writeTextureDataPipelineState);
    TestFreeGpuTexture(gpuTexture);
    TestFreeGpuBuffer(readbackBuffer);
    ElemFreeCommandQueue(commandQueue);
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_LOG_NOERROR();
    ASSERT_BARRIER(dispatch1);
    ASSERT_BARRIER(dispatch2);

    auto floatData = (float*)bufferData.Items;

    for (uint32_t i = 0; i < height; i++)
    {
        for (uint32_t j = 0; j < width; j++)
        {
            auto pixelIndex = i * width * 4 + j * 4;

            ASSERT_EQ_MSG(floatData[pixelIndex], j, "Compute shader data is invalid.");
            ASSERT_EQ_MSG(floatData[pixelIndex + 1], i, "Compute shader data is invalid.");
            ASSERT_EQ_MSG(floatData[pixelIndex + 2], 0.5f, "Compute shader data is invalid.");
            ASSERT_EQ_MSG(floatData[pixelIndex + 3], 1.0f, "Compute shader data is invalid.");
        }
    }
}

UTEST(ResourceBarrier, GraphicsResourceBarrier_TextureRenderTargetAfterWrite) 
{
    // Arrange
    uint32_t width = 16;
    uint32_t height = 16;
    uint32_t threadSize = 16;
    uint32_t dispatchX = (width + (threadSize - 1)) / threadSize;
    uint32_t dispatchY = (height + (threadSize - 1)) / threadSize;
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);
    auto commandQueue = ElemCreateCommandQueue(graphicsDevice, ElemCommandQueueType_Graphics, nullptr);

    auto gpuTexture = TestCreateGpuTexture(graphicsDevice, width, height, ElemGraphicsFormat_R16G16B16A16_FLOAT, (ElemGraphicsResourceUsage)(ElemGraphicsResourceUsage_RenderTarget | ElemGraphicsResourceUsage_Write));
    auto readbackBuffer = TestCreateGpuBuffer(graphicsDevice, width * height * 4 * sizeof(float), ElemGraphicsHeapType_Readback);

    auto writeTextureDataPipelineState = TestOpenComputeShader(graphicsDevice, "ResourceBarrierTests.shader", "TestWriteTextureData");
    auto readTextureDataPipelineState = TestOpenComputeShader(graphicsDevice, "ResourceBarrierTests.shader", "TestReadTextureData");
    auto meshShaderPipelineState = TestOpenMeshShader(graphicsDevice, "ResourceBarrierTests.shader", "TestMeshShader", "TestPixelShader", gpuTexture.Format);

    // Act
    auto commandList = ElemGetCommandList(commandQueue, nullptr);

    ElemGraphicsResourceBarrier(commandList, gpuTexture.WriteDescriptor, nullptr);
    TestDispatchCompute(commandList, writeTextureDataPipelineState, dispatchX, dispatchY, 1, { gpuTexture.WriteDescriptor, 0, 0 });

    INIT_ASSERT_BARRIER(dispatch1, BARRIER_ARRAY_EMPTY(), BARRIER_ARRAY(
        TEXTURE_BARRIER(gpuTexture.Texture, 
                        ElemGraphicsResourceBarrierSyncType_None, ElemGraphicsResourceBarrierSyncType_Compute, 
                        ElemGraphicsResourceBarrierAccessType_NoAccess, ElemGraphicsResourceBarrierAccessType_Write, 
                        ElemGraphicsResourceBarrierLayoutType_Undefined, ElemGraphicsResourceBarrierLayoutType_Write)
    ));

    ElemRenderPassRenderTarget renderPassRenderTarget = 
    {
        .RenderTarget = gpuTexture.Texture,
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
    ElemBindPipelineState(commandList, meshShaderPipelineState);
    ElemDispatchMesh(commandList, 1, 1, 1);
    ElemEndRenderPass(commandList);

    INIT_ASSERT_BARRIER(dispatchMesh, BARRIER_ARRAY_EMPTY(), BARRIER_ARRAY(
        TEXTURE_BARRIER(gpuTexture.Texture, 
                        ElemGraphicsResourceBarrierSyncType_Compute, ElemGraphicsResourceBarrierSyncType_RenderTarget, 
                        ElemGraphicsResourceBarrierAccessType_Write, ElemGraphicsResourceBarrierAccessType_RenderTarget, 
                        ElemGraphicsResourceBarrierLayoutType_Write, ElemGraphicsResourceBarrierLayoutType_RenderTarget)
    ));
    
    ElemGraphicsResourceBarrier(commandList, gpuTexture.ReadDescriptor, nullptr);
    TestDispatchCompute(commandList, readTextureDataPipelineState, dispatchX, dispatchY, 1, { gpuTexture.ReadDescriptor, readbackBuffer.WriteDescriptor, 0 });

    INIT_ASSERT_BARRIER(dispatch2, BARRIER_ARRAY_EMPTY(), BARRIER_ARRAY(
        TEXTURE_BARRIER(gpuTexture.Texture, 
                        ElemGraphicsResourceBarrierSyncType_RenderTarget, ElemGraphicsResourceBarrierSyncType_Compute, 
                        ElemGraphicsResourceBarrierAccessType_RenderTarget, ElemGraphicsResourceBarrierAccessType_Read, 
                        ElemGraphicsResourceBarrierLayoutType_RenderTarget, ElemGraphicsResourceBarrierLayoutType_Read)
    ));

    ElemCommitCommandList(commandList);
    auto fence = ElemExecuteCommandList(commandQueue, commandList, nullptr);

    // Assert
    ElemWaitForFenceOnCpu(fence);
    auto bufferData = ElemGetGraphicsResourceDataSpan(readbackBuffer.Buffer);

    ElemFreePipelineState(readTextureDataPipelineState);
    ElemFreePipelineState(writeTextureDataPipelineState);
    ElemFreePipelineState(meshShaderPipelineState);
    TestFreeGpuTexture(gpuTexture);
    TestFreeGpuBuffer(readbackBuffer);
    ElemFreeCommandQueue(commandQueue);
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_LOG_NOERROR();
    ASSERT_BARRIER(dispatch1);
    ASSERT_BARRIER(dispatchMesh);
    ASSERT_BARRIER(dispatch2);

    auto floatData = (float*)bufferData.Items;

    for (uint32_t i = 0; i < height; i++)
    {
        for (uint32_t j = 0; j < width; j++)
        {
            auto pixelIndex = i * width * 4 + j * 4;

            ASSERT_EQ_MSG(floatData[pixelIndex], 1.0f, "Red channel data is invalid.");
            ASSERT_EQ_MSG(floatData[pixelIndex + 1], 1.0f, "Green channel data is invalid.");
            ASSERT_EQ_MSG(floatData[pixelIndex + 2], 0.0f, "Blue channel data is invalid.");
            ASSERT_EQ_MSG(floatData[pixelIndex + 3], 1.0f, "Alpha channel data is invalid.");
        }
    }
}

// TODO: DepthBuffer

UTEST(ResourceBarrier, GraphicsResourceBarrier_BufferReadAfterWriteWithCustomBeforeSyncAndAccess) 
{
    // Arrange
    int32_t elementCount = 1000000;
    uint32_t threadSize = 16;
    uint32_t dispatchX = (elementCount + (threadSize - 1)) / threadSize;
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);
    auto commandQueue = ElemCreateCommandQueue(graphicsDevice, ElemCommandQueueType_Graphics, nullptr);

    auto gpuBuffer = TestCreateGpuBuffer(graphicsDevice, elementCount * sizeof(uint32_t));
    auto readbackBuffer = TestCreateGpuBuffer(graphicsDevice, elementCount * sizeof(uint32_t), ElemGraphicsHeapType_Readback);

    auto writeBufferDataPipelineState = TestOpenComputeShader(graphicsDevice, "ResourceBarrierTests.shader", "TestWriteBufferData");
    auto readBufferDataPipelineState = TestOpenComputeShader(graphicsDevice, "ResourceBarrierTests.shader", "TestReadBufferData");

    // Act
    auto commandList = ElemGetCommandList(commandQueue, nullptr);

    ElemGraphicsResourceBarrierOptions barrierOptions = 
    {
        .BeforeSync = ElemGraphicsResourceBarrierSyncType_Compute,
        .BeforeAccess = ElemGraphicsResourceBarrierAccessType_Read
    };

    ElemGraphicsResourceBarrier(commandList, gpuBuffer.WriteDescriptor, &barrierOptions);
    TestDispatchCompute(commandList, writeBufferDataPipelineState, dispatchX, 1, 1, { gpuBuffer.WriteDescriptor, 0, elementCount });

    INIT_ASSERT_BARRIER(dispatch1, BARRIER_ARRAY(
        BUFFER_BARRIER(gpuBuffer.Buffer, 
                       ElemGraphicsResourceBarrierSyncType_Compute, ElemGraphicsResourceBarrierSyncType_Compute, 
                       ElemGraphicsResourceBarrierAccessType_Read, ElemGraphicsResourceBarrierAccessType_Write)
    ), BARRIER_ARRAY_EMPTY());

    ElemGraphicsResourceBarrier(commandList, gpuBuffer.ReadDescriptor, nullptr);
    TestDispatchCompute(commandList, readBufferDataPipelineState, dispatchX, 1, 1, { gpuBuffer.ReadDescriptor, readbackBuffer.WriteDescriptor, elementCount });

    INIT_ASSERT_BARRIER(dispatch2, BARRIER_ARRAY(
        BUFFER_BARRIER(gpuBuffer.Buffer, 
                       ElemGraphicsResourceBarrierSyncType_Compute, ElemGraphicsResourceBarrierSyncType_Compute, 
                       ElemGraphicsResourceBarrierAccessType_Write, ElemGraphicsResourceBarrierAccessType_Read)
    ), BARRIER_ARRAY_EMPTY());

    ElemCommitCommandList(commandList);
    auto fence = ElemExecuteCommandList(commandQueue, commandList, nullptr);

    // Assert
    ElemWaitForFenceOnCpu(fence);

    ElemFreePipelineState(readBufferDataPipelineState);
    ElemFreePipelineState(writeBufferDataPipelineState);
    TestFreeGpuBuffer(gpuBuffer);
    TestFreeGpuBuffer(readbackBuffer);
    ElemFreeCommandQueue(commandQueue);
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_LOG_NOERROR();
    ASSERT_BARRIER(dispatch1);
    ASSERT_BARRIER(dispatch2);
}

UTEST(ResourceBarrier, GraphicsResourceBarrier_BufferReadAfterWriteWithCustomAfterSyncAndAccess) 
{
    // Arrange
    int32_t elementCount = 1000000;
    uint32_t threadSize = 16;
    uint32_t dispatchX = (elementCount + (threadSize - 1)) / threadSize;
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);
    auto commandQueue = ElemCreateCommandQueue(graphicsDevice, ElemCommandQueueType_Graphics, nullptr);

    auto gpuBuffer = TestCreateGpuBuffer(graphicsDevice, elementCount * sizeof(uint32_t));
    auto readbackBuffer = TestCreateGpuBuffer(graphicsDevice, elementCount * sizeof(uint32_t), ElemGraphicsHeapType_Readback);

    auto writeBufferDataPipelineState = TestOpenComputeShader(graphicsDevice, "ResourceBarrierTests.shader", "TestWriteBufferData");
    auto readBufferDataPipelineState = TestOpenComputeShader(graphicsDevice, "ResourceBarrierTests.shader", "TestReadBufferData");

    // Act
    auto commandList = ElemGetCommandList(commandQueue, nullptr);

    ElemGraphicsResourceBarrier(commandList, gpuBuffer.WriteDescriptor, nullptr);
    TestDispatchCompute(commandList, writeBufferDataPipelineState, dispatchX, 1, 1, { gpuBuffer.WriteDescriptor, 0, elementCount });

    INIT_ASSERT_BARRIER(dispatch1, BARRIER_ARRAY(
        BUFFER_BARRIER(gpuBuffer.Buffer, 
                       ElemGraphicsResourceBarrierSyncType_None, ElemGraphicsResourceBarrierSyncType_Compute, 
                       ElemGraphicsResourceBarrierAccessType_NoAccess, ElemGraphicsResourceBarrierAccessType_Write)
    ), BARRIER_ARRAY_EMPTY());

    ElemGraphicsResourceBarrierOptions barrierOptions = 
    {
        .AfterSync = ElemGraphicsResourceBarrierSyncType_RenderTarget,
        .AfterAccess = ElemGraphicsResourceBarrierAccessType_Read
    };

    ElemGraphicsResourceBarrier(commandList, gpuBuffer.ReadDescriptor, &barrierOptions);
    TestDispatchCompute(commandList, readBufferDataPipelineState, dispatchX, 1, 1, { gpuBuffer.ReadDescriptor, readbackBuffer.WriteDescriptor, elementCount });

    INIT_ASSERT_BARRIER(dispatch2, BARRIER_ARRAY(
        BUFFER_BARRIER(gpuBuffer.Buffer, 
                       ElemGraphicsResourceBarrierSyncType_Compute, ElemGraphicsResourceBarrierSyncType_RenderTarget, 
                       ElemGraphicsResourceBarrierAccessType_Write, ElemGraphicsResourceBarrierAccessType_Read)
    ), BARRIER_ARRAY_EMPTY());

    ElemCommitCommandList(commandList);
    auto fence = ElemExecuteCommandList(commandQueue, commandList, nullptr);

    // Assert
    ElemWaitForFenceOnCpu(fence);

    ElemFreePipelineState(readBufferDataPipelineState);
    ElemFreePipelineState(writeBufferDataPipelineState);
    TestFreeGpuBuffer(gpuBuffer);
    TestFreeGpuBuffer(readbackBuffer);
    ElemFreeCommandQueue(commandQueue);
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_LOG_NOERROR();
    ASSERT_BARRIER(dispatch1);
    ASSERT_BARRIER(dispatch2);
}

UTEST(ResourceBarrier, GraphicsResourceBarrier_TextureReadAfterWriteWithCustomBeforeSyncAccessAndLayout) 
{
    // Arrange
    uint32_t width = 16;
    uint32_t height = 16;
    uint32_t threadSize = 16;
    uint32_t dispatchX = (width + (threadSize - 1)) / threadSize;
    uint32_t dispatchY = (height + (threadSize - 1)) / threadSize;
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);
    auto commandQueue = ElemCreateCommandQueue(graphicsDevice, ElemCommandQueueType_Graphics, nullptr);

    auto gpuTexture = TestCreateGpuTexture(graphicsDevice, width, height, ElemGraphicsFormat_R16G16B16A16_FLOAT, (ElemGraphicsResourceUsage)(ElemGraphicsResourceUsage_RenderTarget | ElemGraphicsResourceUsage_Write));
    auto readbackBuffer = TestCreateGpuBuffer(graphicsDevice, width * height * 4 * sizeof(float), ElemGraphicsHeapType_Readback);

    auto writeTextureDataPipelineState = TestOpenComputeShader(graphicsDevice, "ResourceBarrierTests.shader", "TestWriteTextureData");
    auto readTextureDataPipelineState = TestOpenComputeShader(graphicsDevice, "ResourceBarrierTests.shader", "TestReadTextureData");

    // Act
    auto previousCommandList = ElemGetCommandList(commandQueue, nullptr);
    ElemGraphicsResourceBarrier(previousCommandList, gpuTexture.ReadDescriptor, nullptr);
    TestDispatchCompute(previousCommandList, writeTextureDataPipelineState, dispatchX, dispatchY, 1, { gpuTexture.WriteDescriptor, 0, 0 });
    ElemCommitCommandList(previousCommandList);

    auto commandList = ElemGetCommandList(commandQueue, nullptr);
    
    ElemGraphicsResourceBarrierOptions barrierOptions = 
    {
        .BeforeSync = ElemGraphicsResourceBarrierSyncType_Compute,
        .BeforeAccess = ElemGraphicsResourceBarrierAccessType_Read,
        .BeforeLayout = ElemGraphicsResourceBarrierLayoutType_Read
    };

    ElemGraphicsResourceBarrier(commandList, gpuTexture.WriteDescriptor, &barrierOptions);
    TestDispatchCompute(commandList, writeTextureDataPipelineState, dispatchX, dispatchY, 1, { gpuTexture.WriteDescriptor, 0, 0 });

    INIT_ASSERT_BARRIER(dispatch1, BARRIER_ARRAY_EMPTY(), BARRIER_ARRAY(
        TEXTURE_BARRIER(gpuTexture.Texture, 
                        ElemGraphicsResourceBarrierSyncType_Compute, ElemGraphicsResourceBarrierSyncType_Compute, 
                        ElemGraphicsResourceBarrierAccessType_Read, ElemGraphicsResourceBarrierAccessType_Write, 
                        ElemGraphicsResourceBarrierLayoutType_Read, ElemGraphicsResourceBarrierLayoutType_Write)
    ));

    ElemGraphicsResourceBarrier(commandList, gpuTexture.ReadDescriptor, nullptr);
    TestDispatchCompute(commandList, readTextureDataPipelineState, dispatchX, dispatchY, 1, { gpuTexture.ReadDescriptor, readbackBuffer.WriteDescriptor, 0 });

    INIT_ASSERT_BARRIER(dispatch2, BARRIER_ARRAY_EMPTY(), BARRIER_ARRAY(
        TEXTURE_BARRIER(gpuTexture.Texture, 
                        ElemGraphicsResourceBarrierSyncType_Compute, ElemGraphicsResourceBarrierSyncType_Compute, 
                        ElemGraphicsResourceBarrierAccessType_Write, ElemGraphicsResourceBarrierAccessType_Read, 
                        ElemGraphicsResourceBarrierLayoutType_Write, ElemGraphicsResourceBarrierLayoutType_Read)
    ));

    ElemCommitCommandList(commandList);
    
    ElemCommandList commandLists[2] = { previousCommandList, commandList };
    auto fence = ElemExecuteCommandLists(commandQueue, { .Items = commandLists, .Length = 2 }, nullptr);

    // Assert
    ElemWaitForFenceOnCpu(fence);

    ElemFreePipelineState(readTextureDataPipelineState);
    ElemFreePipelineState(writeTextureDataPipelineState);
    TestFreeGpuTexture(gpuTexture);
    TestFreeGpuBuffer(readbackBuffer);
    ElemFreeCommandQueue(commandQueue);
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_LOG_NOERROR();
    ASSERT_BARRIER(dispatch1);
    ASSERT_BARRIER(dispatch2);
}

UTEST(ResourceBarrier, GraphicsResourceBarrier_TextureReadAfterWriteWithCustomAfterSyncAccessAndLayout) 
{
    // Arrange
    uint32_t width = 16;
    uint32_t height = 16;
    uint32_t threadSize = 16;
    uint32_t dispatchX = (width + (threadSize - 1)) / threadSize;
    uint32_t dispatchY = (height + (threadSize - 1)) / threadSize;
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);
    auto commandQueue = ElemCreateCommandQueue(graphicsDevice, ElemCommandQueueType_Graphics, nullptr);

    auto gpuTexture = TestCreateGpuTexture(graphicsDevice, width, height, ElemGraphicsFormat_R16G16B16A16_FLOAT, (ElemGraphicsResourceUsage)(ElemGraphicsResourceUsage_RenderTarget | ElemGraphicsResourceUsage_Write));
    auto readbackBuffer = TestCreateGpuBuffer(graphicsDevice, width * height * 4 * sizeof(float), ElemGraphicsHeapType_Readback);

    auto writeTextureDataPipelineState = TestOpenComputeShader(graphicsDevice, "ResourceBarrierTests.shader", "TestWriteTextureData");
    auto readTextureDataPipelineState = TestOpenComputeShader(graphicsDevice, "ResourceBarrierTests.shader", "TestReadTextureData");

    // Act
    auto commandList = ElemGetCommandList(commandQueue, nullptr);

    ElemGraphicsResourceBarrier(commandList, gpuTexture.WriteDescriptor, nullptr);
    TestDispatchCompute(commandList, writeTextureDataPipelineState, dispatchX, dispatchY, 1, { gpuTexture.WriteDescriptor, 0, 0 });

    INIT_ASSERT_BARRIER(dispatch1, BARRIER_ARRAY_EMPTY(), BARRIER_ARRAY(
        TEXTURE_BARRIER(gpuTexture.Texture, 
                        ElemGraphicsResourceBarrierSyncType_None, ElemGraphicsResourceBarrierSyncType_Compute, 
                        ElemGraphicsResourceBarrierAccessType_NoAccess, ElemGraphicsResourceBarrierAccessType_Write, 
                        ElemGraphicsResourceBarrierLayoutType_Undefined, ElemGraphicsResourceBarrierLayoutType_Write)
    ));
    
    ElemGraphicsResourceBarrierOptions barrierOptions = 
    {
        .AfterSync = ElemGraphicsResourceBarrierSyncType_RenderTarget,
        .AfterAccess = ElemGraphicsResourceBarrierAccessType_RenderTarget,
        .AfterLayout = ElemGraphicsResourceBarrierLayoutType_RenderTarget
    };

    ElemGraphicsResourceBarrier(commandList, gpuTexture.ReadDescriptor, &barrierOptions);
    TestDispatchCompute(commandList, readTextureDataPipelineState, dispatchX, dispatchY, 1, { gpuTexture.ReadDescriptor, readbackBuffer.WriteDescriptor, 0 });

    INIT_ASSERT_BARRIER(dispatch2, BARRIER_ARRAY_EMPTY(), BARRIER_ARRAY(
        TEXTURE_BARRIER(gpuTexture.Texture, 
                        ElemGraphicsResourceBarrierSyncType_Compute, ElemGraphicsResourceBarrierSyncType_RenderTarget, 
                        ElemGraphicsResourceBarrierAccessType_Write, ElemGraphicsResourceBarrierAccessType_RenderTarget, 
                        ElemGraphicsResourceBarrierLayoutType_Write, ElemGraphicsResourceBarrierLayoutType_RenderTarget)
    ));

    ElemCommitCommandList(commandList);
    auto fence = ElemExecuteCommandList(commandQueue, commandList, nullptr);

    // Assert
    ElemWaitForFenceOnCpu(fence);

    ElemFreePipelineState(readTextureDataPipelineState);
    ElemFreePipelineState(writeTextureDataPipelineState);
    TestFreeGpuTexture(gpuTexture);
    TestFreeGpuBuffer(readbackBuffer);
    ElemFreeCommandQueue(commandQueue);
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_LOG_NOERROR();
    ASSERT_BARRIER(dispatch1);
    ASSERT_BARRIER(dispatch2);
}

UTEST(ResourceBarrier, GraphicsResourceBarrier_BufferReadAfterWriteMultiCommandList) 
{
    // Arrange
    int32_t elementCount = 1000000;
    uint32_t threadSize = 16;
    uint32_t dispatchX = (elementCount + (threadSize - 1)) / threadSize;
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);
    auto commandQueue = ElemCreateCommandQueue(graphicsDevice, ElemCommandQueueType_Graphics, nullptr);

    auto gpuBuffer = TestCreateGpuBuffer(graphicsDevice, elementCount * sizeof(uint32_t));
    auto readbackBuffer = TestCreateGpuBuffer(graphicsDevice, elementCount * sizeof(uint32_t), ElemGraphicsHeapType_Readback);

    auto writeBufferDataPipelineState = TestOpenComputeShader(graphicsDevice, "ResourceBarrierTests.shader", "TestWriteBufferData");
    auto readBufferDataPipelineState = TestOpenComputeShader(graphicsDevice, "ResourceBarrierTests.shader", "TestReadBufferData");

    // Act
    auto commandList = ElemGetCommandList(commandQueue, nullptr);

    ElemGraphicsResourceBarrier(commandList, gpuBuffer.WriteDescriptor, nullptr);
    TestDispatchCompute(commandList, writeBufferDataPipelineState, dispatchX, 1, 1, { gpuBuffer.WriteDescriptor, 0, elementCount });

    INIT_ASSERT_BARRIER(dispatch1, BARRIER_ARRAY(
        BUFFER_BARRIER(gpuBuffer.Buffer, 
                       ElemGraphicsResourceBarrierSyncType_None, ElemGraphicsResourceBarrierSyncType_Compute, 
                       ElemGraphicsResourceBarrierAccessType_NoAccess, ElemGraphicsResourceBarrierAccessType_Write)
    ), BARRIER_ARRAY_EMPTY());

    ElemCommitCommandList(commandList);

    auto commandList2 = ElemGetCommandList(commandQueue, nullptr);

    ElemGraphicsResourceBarrierOptions barrierOptions = 
    {
        .BeforeSync = ElemGraphicsResourceBarrierSyncType_Compute,
        .BeforeAccess = ElemGraphicsResourceBarrierAccessType_Write
    };

    ElemGraphicsResourceBarrier(commandList2, gpuBuffer.ReadDescriptor, &barrierOptions);
    TestDispatchCompute(commandList2, readBufferDataPipelineState, dispatchX, 1, 1, { gpuBuffer.ReadDescriptor, readbackBuffer.WriteDescriptor, elementCount });

    INIT_ASSERT_BARRIER(dispatch2, BARRIER_ARRAY(
        BUFFER_BARRIER(gpuBuffer.Buffer, 
                       ElemGraphicsResourceBarrierSyncType_Compute, ElemGraphicsResourceBarrierSyncType_Compute, 
                       ElemGraphicsResourceBarrierAccessType_Write, ElemGraphicsResourceBarrierAccessType_Read)
    ), BARRIER_ARRAY_EMPTY());

    ElemCommitCommandList(commandList2);
    ElemCommandList commandLists[2] = { commandList, commandList2 };
    auto fence = ElemExecuteCommandLists(commandQueue, { .Items = commandLists, .Length = 2 }, nullptr);

    // Assert
    ElemWaitForFenceOnCpu(fence);
    auto bufferData = ElemGetGraphicsResourceDataSpan(readbackBuffer.Buffer);

    ElemFreePipelineState(readBufferDataPipelineState);
    ElemFreePipelineState(writeBufferDataPipelineState);
    TestFreeGpuBuffer(gpuBuffer);
    TestFreeGpuBuffer(readbackBuffer);
    ElemFreeCommandQueue(commandQueue);
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_LOG_NOERROR();
    ASSERT_BARRIER(dispatch1);
    ASSERT_BARRIER(dispatch2);

    auto intData = (int32_t*)bufferData.Items;

    for (int32_t i = 0; i < elementCount; i++)
    {
        ASSERT_EQ_MSG(intData[i], elementCount - i - 1, "Compute shader data is invalid.");
    }
}
