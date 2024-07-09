#include "Elemental.h"
#include "GraphicsTests.h"
#include "utest.h"

// TODO: Test barriers with compute/render encoders and different parameters
// TODO: Test barriers inside the same encoder and different encoder
// TODO: Test Write to write barriers
// TODO: Test multiple resources

UTEST(Resource, GraphicsResourceBarrier_BufferReadAfterWrite) 
{
    // Arrange
    int32_t elementCount = 1000000;
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);
    auto commandQueue = ElemCreateCommandQueue(graphicsDevice, ElemCommandQueueType_Graphics, nullptr);

    auto graphicsHeap = ElemCreateGraphicsHeap(graphicsDevice, TestMegaBytesToBytes(8), nullptr);

    auto gpuBufferInfo = ElemCreateGraphicsBufferResourceInfo(graphicsDevice, elementCount * sizeof(uint32_t), ElemGraphicsResourceUsage_Write, nullptr);
    auto gpuBuffer = ElemCreateGraphicsResource(graphicsHeap, 0, &gpuBufferInfo);
    auto gpuBufferReadDescriptor = ElemCreateGraphicsResourceDescriptor(gpuBuffer, ElemGraphicsResourceDescriptorUsage_Read, nullptr);
    auto gpuBufferWriteDescriptor = ElemCreateGraphicsResourceDescriptor(gpuBuffer, ElemGraphicsResourceDescriptorUsage_Write, nullptr);

    auto readbackBuffer = TestCreateReadbackBuffer(graphicsDevice, elementCount * sizeof(uint32_t));

    auto writeBufferDataPipelineState = TestOpenComputeShader(graphicsDevice, "ResourceBarrierTests.shader", "TestWriteBufferData");
    auto readBufferDataPipelineState = TestOpenComputeShader(graphicsDevice, "ResourceBarrierTests.shader", "TestReadBufferData");

    int32_t writeParameters[] = { gpuBufferWriteDescriptor, 0, elementCount };
    int32_t readParameters[] = { gpuBufferReadDescriptor, readbackBuffer.Descriptor, elementCount };

    uint32_t threadSize = 16;

    // Act
    auto commandList = ElemGetCommandList(commandQueue, nullptr);

    ElemGraphicsResourceBarrier(commandList, gpuBufferWriteDescriptor, nullptr);

    ElemBindPipelineState(commandList, writeBufferDataPipelineState);
    ElemPushPipelineStateConstants(commandList, 0, { .Items = (uint8_t*)writeParameters, .Length = ARRAYSIZE(writeParameters) * sizeof(int32_t) });
    ElemDispatchCompute(commandList, (elementCount + (threadSize - 1)) / threadSize, 1, 1);

    INIT_ASSERT_BARRIER(dispatch1, BARRIER_ARRAY(
        BUFFER_BARRIER(gpuBuffer, SyncType_None, SyncType_Compute, AccessType_NoAccess, AccessType_Write)
    ));

    ElemGraphicsResourceBarrier(commandList, gpuBufferReadDescriptor, nullptr);

    ElemBindPipelineState(commandList, readBufferDataPipelineState);
    ElemPushPipelineStateConstants(commandList, 0, { .Items = (uint8_t*)readParameters, .Length = ARRAYSIZE(readParameters) * sizeof(int32_t) });
    ElemDispatchCompute(commandList, (elementCount + (threadSize - 1)) / threadSize, 1, 1);

    INIT_ASSERT_BARRIER(dispatch2, BARRIER_ARRAY(
        BUFFER_BARRIER(gpuBuffer, SyncType_Compute, SyncType_Compute, AccessType_Write, AccessType_Read)
    ));

    ElemCommitCommandList(commandList);
    auto fence = ElemExecuteCommandList(commandQueue, commandList, nullptr);

    // Assert
    ElemWaitForFenceOnCpu(fence);
    auto bufferData = ElemGetGraphicsResourceDataSpan(readbackBuffer.Buffer);

    ElemFreePipelineState(readBufferDataPipelineState);
    ElemFreePipelineState(writeBufferDataPipelineState);
    TestFreeReadbackBuffer(readbackBuffer);
    ElemFreeGraphicsResource(gpuBuffer, nullptr);
    ElemFreeGraphicsHeap(graphicsHeap);
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

UTEST(Resource, GraphicsResourceBarrier_BufferWriteAfterWrite) 
{
    // Arrange
    int32_t elementCount = 1000000;
    int32_t addOffset = 28;

    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);
    auto commandQueue = ElemCreateCommandQueue(graphicsDevice, ElemCommandQueueType_Graphics, nullptr);

    auto graphicsHeap = ElemCreateGraphicsHeap(graphicsDevice, TestMegaBytesToBytes(8), nullptr);

    auto gpuBufferInfo = ElemCreateGraphicsBufferResourceInfo(graphicsDevice, elementCount * sizeof(uint32_t), ElemGraphicsResourceUsage_Write, nullptr);
    auto gpuBuffer = ElemCreateGraphicsResource(graphicsHeap, 0, &gpuBufferInfo);
    auto gpuBufferReadDescriptor = ElemCreateGraphicsResourceDescriptor(gpuBuffer, ElemGraphicsResourceDescriptorUsage_Read, nullptr);
    auto gpuBufferWriteDescriptor = ElemCreateGraphicsResourceDescriptor(gpuBuffer, ElemGraphicsResourceDescriptorUsage_Write, nullptr);

    auto readbackBuffer = TestCreateReadbackBuffer(graphicsDevice, elementCount * sizeof(uint32_t));

    auto writeBufferDataPipelineState = TestOpenComputeShader(graphicsDevice, "ResourceBarrierTests.shader", "TestWriteBufferData");
    auto writeAddBufferDataPipelineState = TestOpenComputeShader(graphicsDevice, "ResourceBarrierTests.shader", "TestWriteAddBufferData");
    auto readBufferDataPipelineState = TestOpenComputeShader(graphicsDevice, "ResourceBarrierTests.shader", "TestReadBufferData");

    int32_t writeParameters[3] = { gpuBufferWriteDescriptor, 0, elementCount };
    int32_t writeAddParameters[3] = { gpuBufferWriteDescriptor, addOffset, elementCount };
    int32_t readParameters[3] = { gpuBufferReadDescriptor, readbackBuffer.Descriptor, elementCount };

    uint32_t threadSize = 16;

    // Act
    auto commandList = ElemGetCommandList(commandQueue, nullptr);

    ElemGraphicsResourceBarrier(commandList, gpuBufferWriteDescriptor, nullptr);

    ElemBindPipelineState(commandList, writeBufferDataPipelineState);
    ElemPushPipelineStateConstants(commandList, 0, { .Items = (uint8_t*)writeParameters, .Length = 3 * sizeof(int32_t) });
    ElemDispatchCompute(commandList, (elementCount + (threadSize - 1)) / threadSize, 1, 1);
   
    ElemGraphicsResourceBarrier(commandList, gpuBufferWriteDescriptor, nullptr);

    ElemBindPipelineState(commandList, writeAddBufferDataPipelineState);
    ElemPushPipelineStateConstants(commandList, 0, { .Items = (uint8_t*)writeAddParameters, .Length = 3 * sizeof(int32_t) });
    ElemDispatchCompute(commandList, (elementCount + (threadSize - 1)) / threadSize, 1, 1);

    // TODO: Assert debug barrier

    ElemGraphicsResourceBarrier(commandList, gpuBufferReadDescriptor, nullptr);

    ElemBindPipelineState(commandList, readBufferDataPipelineState);
    ElemPushPipelineStateConstants(commandList, 0, { .Items = (uint8_t*)readParameters, .Length = 3 * sizeof(int32_t) });
    ElemDispatchCompute(commandList, (elementCount + (threadSize - 1)) / threadSize, 1, 1);

    ElemCommitCommandList(commandList);
    auto fence = ElemExecuteCommandList(commandQueue, commandList, nullptr);

    // Assert
    ElemWaitForFenceOnCpu(fence);
    auto bufferData = ElemGetGraphicsResourceDataSpan(readbackBuffer.Buffer);

    ElemFreePipelineState(readBufferDataPipelineState);
    ElemFreePipelineState(writeAddBufferDataPipelineState);
    ElemFreePipelineState(writeBufferDataPipelineState);
    TestFreeReadbackBuffer(readbackBuffer);
    ElemFreeGraphicsResource(gpuBuffer, nullptr);
    ElemFreeGraphicsHeap(graphicsHeap);
    ElemFreeCommandQueue(commandQueue);
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_LOG_NOERROR();
    auto intData = (int32_t*)bufferData.Items;

    for (int32_t i = 0; i < elementCount; i++)
    {
        ASSERT_EQ_MSG(intData[i], elementCount - i - 1 + addOffset, "Compute shader data is invalid.");
    }
}
