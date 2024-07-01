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
