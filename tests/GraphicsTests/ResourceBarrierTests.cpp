#include "Elemental.h"
#include "GraphicsTests.h"
#include "utest.h"

// TODO: Test barriers with compute/render encoders and different parameters
// TODO: Test barriers inside the same encoder and different encoder
// TODO: Test Buffer/Texture barriers

// TODO: Test All commands that can insert barriers.
//  Candidates:
//      - DispatchCompute
//      - DispatchMesh
//      - BeginRenderPass
//      - EndRenderPass

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
        BUFFER_BARRIER(gpuBuffer.Buffer, SyncType_None, SyncType_Compute, AccessType_NoAccess, AccessType_Write)
    ), BARRIER_ARRAY_EMPTY());

    ElemGraphicsResourceBarrier(commandList, gpuBuffer.ReadDescriptor, nullptr);
    TestDispatchCompute(commandList, readBufferDataPipelineState, dispatchX, 1, 1, { gpuBuffer.ReadDescriptor, readbackBuffer.WriteDescriptor, elementCount });

    INIT_ASSERT_BARRIER(dispatch2, BARRIER_ARRAY(
        BUFFER_BARRIER(gpuBuffer.Buffer, SyncType_Compute, SyncType_Compute, AccessType_Write, AccessType_Read)
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
        BUFFER_BARRIER(gpuBuffer.Buffer, SyncType_None, SyncType_Compute, AccessType_NoAccess, AccessType_Write)
    ), BARRIER_ARRAY_EMPTY());
   
    ElemGraphicsResourceBarrier(commandList, gpuBuffer.WriteDescriptor, nullptr);
    TestDispatchCompute(commandList, writeAddBufferDataPipelineState, dispatchX, 1, 1, { gpuBuffer.WriteDescriptor, addOffset, elementCount });

    INIT_ASSERT_BARRIER(dispatch2, BARRIER_ARRAY(
        BUFFER_BARRIER(gpuBuffer.Buffer, SyncType_Compute, SyncType_Compute, AccessType_Write, AccessType_Write)
    ), BARRIER_ARRAY_EMPTY());

    ElemGraphicsResourceBarrier(commandList, gpuBuffer.ReadDescriptor, nullptr);
    TestDispatchCompute(commandList, readBufferDataPipelineState, dispatchX, 1, 1, { gpuBuffer.ReadDescriptor, readbackBuffer.WriteDescriptor, elementCount });

    INIT_ASSERT_BARRIER(dispatch3, BARRIER_ARRAY(
        BUFFER_BARRIER(gpuBuffer.Buffer, SyncType_Compute, SyncType_Compute, AccessType_Write, AccessType_Read)
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
        BUFFER_BARRIER(gpuBuffer.Buffer, SyncType_None, SyncType_Compute, AccessType_NoAccess, AccessType_Write)
    ), BARRIER_ARRAY_EMPTY());
   
    ElemGraphicsResourceBarrier(commandList, gpuBuffer.ReadDescriptor, nullptr);
    ElemGraphicsResourceBarrier(commandList, gpuBuffer2.WriteDescriptor, nullptr);
    TestDispatchCompute(commandList, readBufferDataPipelineState, dispatchX, 1, 1, { gpuBuffer.ReadDescriptor, gpuBuffer2.WriteDescriptor, elementCount });

    INIT_ASSERT_BARRIER(dispatch2, BARRIER_ARRAY(
        BUFFER_BARRIER(gpuBuffer.Buffer, SyncType_Compute, SyncType_Compute, AccessType_Write, AccessType_Read),
        BUFFER_BARRIER(gpuBuffer2.Buffer, SyncType_None, SyncType_Compute, AccessType_NoAccess, AccessType_Write)
    ), BARRIER_ARRAY_EMPTY());

    ElemGraphicsResourceBarrier(commandList, gpuBuffer2.ReadDescriptor, nullptr);
    TestDispatchCompute(commandList, readBufferDataPipelineState, dispatchX, 1, 1, { gpuBuffer2.ReadDescriptor, readbackBuffer.WriteDescriptor, elementCount });

    INIT_ASSERT_BARRIER(dispatch3, BARRIER_ARRAY(
        BUFFER_BARRIER(gpuBuffer2.Buffer, SyncType_Compute, SyncType_Compute, AccessType_Write, AccessType_Read)
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

    auto gpuTexture = TestCreateGpuTexture(graphicsDevice, width, height, ElemGraphicsFormat_R16G16B16A16_FLOAT);
    auto readbackBuffer = TestCreateGpuBuffer(graphicsDevice, width * height * 4 * sizeof(float), ElemGraphicsHeapType_Readback);

    auto writeTextureDataPipelineState = TestOpenComputeShader(graphicsDevice, "ResourceBarrierTests.shader", "TestWriteTextureData");
    auto readTextureDataPipelineState = TestOpenComputeShader(graphicsDevice, "ResourceBarrierTests.shader", "TestReadTextureData");

    // Act
    auto commandList = ElemGetCommandList(commandQueue, nullptr);

    ElemGraphicsResourceBarrier(commandList, gpuTexture.WriteDescriptor, nullptr);
    TestDispatchCompute(commandList, writeTextureDataPipelineState, dispatchX, dispatchY, 1, { gpuTexture.WriteDescriptor, 0, 0 });

    INIT_ASSERT_BARRIER(dispatch1, BARRIER_ARRAY_EMPTY(), BARRIER_ARRAY(
        TEXTURE_BARRIER(gpuTexture.Texture, SyncType_None, SyncType_Compute, AccessType_NoAccess, AccessType_Write, LayoutType_Undefined, LayoutType_Write)
    ));

    ElemGraphicsResourceBarrier(commandList, gpuTexture.ReadDescriptor, nullptr);
    TestDispatchCompute(commandList, readTextureDataPipelineState, dispatchX, dispatchY, 1, { gpuTexture.ReadDescriptor, readbackBuffer.WriteDescriptor, 0 });

    INIT_ASSERT_BARRIER(dispatch2, BARRIER_ARRAY_EMPTY(), BARRIER_ARRAY(
        TEXTURE_BARRIER(gpuTexture.Texture, SyncType_Compute, SyncType_Compute, AccessType_Write, AccessType_Read, LayoutType_Write, LayoutType_Read)
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

