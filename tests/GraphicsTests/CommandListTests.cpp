#include "Elemental.h"
#include "GraphicsTests.h"
#include "utest.h"

UTEST(CommandList, GetCommandList) 
{
    // Arrange
    InitLog();
    auto graphicsDevice = GetSharedGraphicsDevice();
    auto commandQueue = ElemCreateCommandQueue(graphicsDevice, ElemCommandQueueType_Graphics, nullptr);

    // Act
    auto commandList = ElemGetCommandList(commandQueue, nullptr);

    // Assert
    ElemCommitCommandList(commandList);

    ASSERT_NE(commandQueue, ELEM_HANDLE_NULL);
    ASSERT_NE(commandList, ELEM_HANDLE_NULL);
    ASSERT_FALSE(testHasLogErrors);

    ElemFreeCommandQueue(commandQueue);
}

UTEST(CommandList, GetCommandListWithoutPreviousCommittedOnSameThread) 
{
    // Arrange
    InitLog();
    auto graphicsDevice = GetSharedGraphicsDevice();
    auto commandQueue = ElemCreateCommandQueue(graphicsDevice, ElemCommandQueueType_Graphics, nullptr);
    ElemGetCommandList(commandQueue, nullptr);

    // Act
    ElemGetCommandList(commandQueue, nullptr);

    // Assert
    ASSERT_TRUE(testHasLogErrors);
    ASSERT_LOG("Cannot get a command list if commit was not called on the same thread.");
    ElemFreeCommandQueue(commandQueue);
}

UTEST(CommandList, ExecuteCommandListWithoutCommit) 
{
    // Arrange
    InitLog();
    auto graphicsDevice = GetSharedGraphicsDevice();
    auto commandQueue = ElemCreateCommandQueue(graphicsDevice, ElemCommandQueueType_Graphics, nullptr);
    auto commandList = ElemGetCommandList(commandQueue, nullptr);

    // Act
    ElemExecuteCommandList(commandQueue, commandList, nullptr);

    // Assert
    ASSERT_TRUE(testHasLogErrors);
    ASSERT_LOG("Commandlist needs to be committed before executing it.");

    ElemFreeCommandQueue(commandQueue);
}

UTEST(CommandList, ExecuteCommandListFenceIsValid) 
{
    // Arrange
    InitLog();
    auto graphicsDevice = GetSharedGraphicsDevice();
    auto commandQueue = ElemCreateCommandQueue(graphicsDevice, ElemCommandQueueType_Graphics, nullptr);
    auto commandList = ElemGetCommandList(commandQueue, nullptr);
    ElemCommitCommandList(commandList);

    // Act
    auto fence = ElemExecuteCommandList(commandQueue, commandList, nullptr);

    // Assert
    ASSERT_FALSE(testHasLogErrors);
    ASSERT_EQ(fence.CommandQueue, commandQueue);
    ASSERT_GT(fence.FenceValue, 0u);

    ElemWaitForFenceOnCpu(fence);
    ElemFreeCommandQueue(commandQueue);
}

// TODO: Test Cannot wait fence on cpu if flag was not passed
// TODO: Test mutli threading create command list
// TODO: Test also one / multi thread with multiple frame in flight (if we don't use swapchain it can grow dynamically)
// TODO: Test Fences, assign a buffer counter at each step and check the result
