#include "Elemental.h"
#include "GraphicsTests.h"
#include "utest.h"

// TODO: Test operation that can execute only on certain queue types
// TODO: Test mutli threading create command list
// TODO: Test also one / multi thread with multiple frame in flight (if we don't use swapchain it can grow dynamically)
// TODO: Test Fences, assign a buffer counter at each step and check the result
// TODO: Test WaitForFenceOnCpu
// TODO: Test IsFenceCompleted()

UTEST(CommandList, GetCommandList) 
{
    // Arrange
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);
    auto commandQueue = ElemCreateCommandQueue(graphicsDevice, ElemCommandQueueType_Graphics, nullptr);
    ElemCommandListOptions options = { .DebugName = "GetCommandList" };

    // Act
    auto commandList = ElemGetCommandList(commandQueue, &options);

    // Assert
    ElemCommitCommandList(commandList);

    ASSERT_NE(commandQueue, ELEM_HANDLE_NULL);
    ASSERT_NE(commandList, ELEM_HANDLE_NULL);
    ASSERT_LOG_NOERROR();

    ElemFreeCommandQueue(commandQueue);
    ElemFreeGraphicsDevice(graphicsDevice);
}

UTEST(CommandList, GetCommandListWithoutPreviousCommittedOnSameThread) 
{
    // Arrange
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);
    auto commandQueue = ElemCreateCommandQueue(graphicsDevice, ElemCommandQueueType_Graphics, nullptr);
    ElemCommandListOptions options = { .DebugName = "GetCommandListWithoutPreviousCommittedOnSameThread" };
    auto commandList1 = ElemGetCommandList(commandQueue, &options);

    // Act
    ElemGetCommandList(commandQueue, &options);

    // Assert
    ASSERT_LOG_MESSAGE("Cannot get a command list if commit was not called on the same thread.");
    ElemCommitCommandList(commandList1);
    ElemFreeCommandQueue(commandQueue);
    ElemFreeGraphicsDevice(graphicsDevice);
}

UTEST(CommandList, ExecuteCommandListWithoutCommit) 
{
    // Arrange
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);
    auto commandQueue = ElemCreateCommandQueue(graphicsDevice, ElemCommandQueueType_Graphics, nullptr);
    ElemCommandListOptions options = { .DebugName = "ExecuteCommandListWithoutCommit" };
    auto commandList = ElemGetCommandList(commandQueue, &options);

    // Act
    ElemExecuteCommandList(commandQueue, commandList, nullptr);

    // Assert
    ASSERT_LOG_MESSAGE("Commandlist needs to be committed before executing it.");
    ElemFreeCommandQueue(commandQueue);
    ElemFreeGraphicsDevice(graphicsDevice);
}

UTEST(CommandList, ExecuteCommandListFenceIsValid) 
{
    // Arrange
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);
    auto commandQueue = ElemCreateCommandQueue(graphicsDevice, ElemCommandQueueType_Graphics, nullptr);
    ElemCommandListOptions options = { .DebugName = "ExecuteCommandListFenceIsValid" };
    auto commandList = ElemGetCommandList(commandQueue, &options);
    ElemCommitCommandList(commandList);

    // Act
    auto fence = ElemExecuteCommandList(commandQueue, commandList, nullptr);

    // Assert
    ASSERT_LOG_NOERROR();
    ASSERT_EQ(fence.CommandQueue, commandQueue);
    ASSERT_GT(fence.FenceValue, 0u);

    ElemFreeCommandQueue(commandQueue);
    ElemFreeGraphicsDevice(graphicsDevice);
}

UTEST(CommandList, ExecuteCommandListWaitForFence) 
{
    // Arrange
    int32_t elementCount = 1000000;
    uint32_t threadSize = 16;
    uint32_t dispatchX = (elementCount + (threadSize - 1)) / threadSize;
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);
    auto commandQueue = ElemCreateCommandQueue(graphicsDevice, ElemCommandQueueType_Graphics, nullptr);

    auto gpuBuffer = TestCreateGpuBuffer(graphicsDevice, elementCount * sizeof(uint32_t));
    auto readbackBuffer = TestCreateGpuBuffer(graphicsDevice, elementCount * sizeof(uint32_t), ElemGraphicsHeapType_Readback);

    auto writeBufferDataPipelineState = TestOpenComputeShader(graphicsDevice, "CommandListTests.shader", "TestWriteBufferData");
    auto readBufferDataPipelineState = TestOpenComputeShader(graphicsDevice, "CommandListTests.shader", "TestReadBufferData");

    // Act
    auto commandList = ElemGetCommandList(commandQueue, nullptr);

    TestDispatchCompute(commandList, writeBufferDataPipelineState, dispatchX, 1, 1, { gpuBuffer.WriteDescriptor, 0, elementCount });
    ElemCommitCommandList(commandList);
    auto testFence = ElemExecuteCommandList(commandQueue, commandList, nullptr);

    auto commandList2 = ElemGetCommandList(commandQueue, nullptr);

    TestDispatchCompute(commandList2, readBufferDataPipelineState, dispatchX, 1, 1, { gpuBuffer.ReadDescriptor, readbackBuffer.WriteDescriptor, elementCount });
    ElemCommitCommandList(commandList2);
    
    ElemExecuteCommandListOptions executeOptions =
    {
        .FencesToWait = { .Items = &testFence, .Length = 1 }
    };

    auto fence = ElemExecuteCommandList(commandQueue, commandList2, &executeOptions);

    // Assert
    ElemWaitForFenceOnCpu(fence);
    auto bufferData = ElemDownloadGraphicsBufferData(readbackBuffer.Buffer, nullptr);

    ElemFreePipelineState(readBufferDataPipelineState);
    ElemFreePipelineState(writeBufferDataPipelineState);
    TestFreeGpuBuffer(gpuBuffer);
    TestFreeGpuBuffer(readbackBuffer);
    ElemFreeCommandQueue(commandQueue);
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_LOG_NOERROR();

    char logString[255];
    snprintf(logString, 255, "Waiting for fence before ExecuteCommandLists. (CommandQueue=%u, Value=%u)", (uint32_t)testFence.CommandQueue, (uint32_t)testFence.FenceValue);
    ASSERT_LOG_MESSAGE_DEBUG(logString);

    auto intData = (int32_t*)bufferData.Items;

    for (int32_t i = 0; i < elementCount; i++)
    {
        ASSERT_EQ_MSG(intData[i], elementCount - i - 1, "Compute shader data is invalid.");
    }
}
