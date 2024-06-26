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
    auto graphicsDevice = TestGetSharedGraphicsDevice();
    auto commandQueue = ElemCreateCommandQueue(graphicsDevice, ElemCommandQueueType_Graphics, nullptr);

    // Act
    auto commandList = ElemGetCommandList(commandQueue, nullptr);

    // Assert
    ElemCommitCommandList(commandList);

    ASSERT_NE(commandQueue, ELEM_HANDLE_NULL);
    ASSERT_NE(commandList, ELEM_HANDLE_NULL);
    ASSERT_LOG_NOERROR();

    ElemFreeCommandQueue(commandQueue);
}

UTEST(CommandList, GetCommandListWithoutPreviousCommittedOnSameThread) 
{
    // Arrange
    auto graphicsDevice = TestGetSharedGraphicsDevice();
    auto commandQueue = ElemCreateCommandQueue(graphicsDevice, ElemCommandQueueType_Graphics, nullptr);
    ElemGetCommandList(commandQueue, nullptr);

    // Act
    ElemGetCommandList(commandQueue, nullptr);

    // Assert
    ASSERT_LOG_MESSAGE("Cannot get a command list if commit was not called on the same thread.");
    ElemFreeCommandQueue(commandQueue);
}

UTEST(CommandList, ExecuteCommandListWithoutCommit) 
{
    // Arrange
    auto graphicsDevice = TestGetSharedGraphicsDevice();
    auto commandQueue = ElemCreateCommandQueue(graphicsDevice, ElemCommandQueueType_Graphics, nullptr);
    auto commandList = ElemGetCommandList(commandQueue, nullptr);

    // Act
    ElemExecuteCommandList(commandQueue, commandList, nullptr);

    // Assert
    ASSERT_LOG_MESSAGE("Commandlist needs to be committed before executing it.");

    ElemFreeCommandQueue(commandQueue);
}

UTEST(CommandList, ExecuteCommandListFenceIsValid) 
{
    // Arrange
    auto graphicsDevice = TestGetSharedGraphicsDevice();
    auto commandQueue = ElemCreateCommandQueue(graphicsDevice, ElemCommandQueueType_Graphics, nullptr);
    auto commandList = ElemGetCommandList(commandQueue, nullptr);
    ElemCommitCommandList(commandList);

    // Act
    auto fence = ElemExecuteCommandList(commandQueue, commandList, nullptr);

    // Assert
    ASSERT_LOG_NOERROR();
    ASSERT_EQ(fence.CommandQueue, commandQueue);
    ASSERT_GT(fence.FenceValue, 0u);

    ElemWaitForFenceOnCpu(fence);
    ElemFreeCommandQueue(commandQueue);
}
