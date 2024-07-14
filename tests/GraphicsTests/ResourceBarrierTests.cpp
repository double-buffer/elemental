#include "Elemental.h"
#include "GraphicsTests.h"
#include "utest.h"

// TODO: Test barriers with compute/render encoders and different parameters
// TODO: Test barriers inside the same encoder and different encoder
// TODO: Test Burrer/Texture barriers

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
    ));

    ElemGraphicsResourceBarrier(commandList, gpuBuffer.ReadDescriptor, nullptr);
    TestDispatchCompute(commandList, readBufferDataPipelineState, dispatchX, 1, 1, { gpuBuffer.ReadDescriptor, readbackBuffer.WriteDescriptor, elementCount });

    INIT_ASSERT_BARRIER(dispatch2, BARRIER_ARRAY(
        BUFFER_BARRIER(gpuBuffer.Buffer, SyncType_Compute, SyncType_Compute, AccessType_Write, AccessType_Read)
    ));

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
    ));
   
    ElemGraphicsResourceBarrier(commandList, gpuBuffer.WriteDescriptor, nullptr);
    TestDispatchCompute(commandList, writeAddBufferDataPipelineState, dispatchX, 1, 1, { gpuBuffer.WriteDescriptor, addOffset, elementCount });

    INIT_ASSERT_BARRIER(dispatch2, BARRIER_ARRAY(
        BUFFER_BARRIER(gpuBuffer.Buffer, SyncType_Compute, SyncType_Compute, AccessType_Write, AccessType_Write)
    ));

    ElemGraphicsResourceBarrier(commandList, gpuBuffer.ReadDescriptor, nullptr);
    TestDispatchCompute(commandList, readBufferDataPipelineState, dispatchX, 1, 1, { gpuBuffer.ReadDescriptor, readbackBuffer.WriteDescriptor, elementCount });

    INIT_ASSERT_BARRIER(dispatch3, BARRIER_ARRAY(
        BUFFER_BARRIER(gpuBuffer.Buffer, SyncType_Compute, SyncType_Compute, AccessType_Write, AccessType_Read)
    ));

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
    ));
   
    ElemGraphicsResourceBarrier(commandList, gpuBuffer.ReadDescriptor, nullptr);
    ElemGraphicsResourceBarrier(commandList, gpuBuffer2.WriteDescriptor, nullptr);
    TestDispatchCompute(commandList, readBufferDataPipelineState, dispatchX, 1, 1, { gpuBuffer.ReadDescriptor, gpuBuffer2.WriteDescriptor, elementCount });

    INIT_ASSERT_BARRIER(dispatch2, BARRIER_ARRAY(
        BUFFER_BARRIER(gpuBuffer.Buffer, SyncType_Compute, SyncType_Compute, AccessType_Write, AccessType_Read),
        BUFFER_BARRIER(gpuBuffer2.Buffer, SyncType_None, SyncType_Compute, AccessType_NoAccess, AccessType_Write)
    ));

    ElemGraphicsResourceBarrier(commandList, gpuBuffer2.ReadDescriptor, nullptr);
    TestDispatchCompute(commandList, readBufferDataPipelineState, dispatchX, 1, 1, { gpuBuffer2.ReadDescriptor, readbackBuffer.WriteDescriptor, elementCount });

    INIT_ASSERT_BARRIER(dispatch3, BARRIER_ARRAY(
        BUFFER_BARRIER(gpuBuffer2.Buffer, SyncType_Compute, SyncType_Compute, AccessType_Write, AccessType_Read)
    ));

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
