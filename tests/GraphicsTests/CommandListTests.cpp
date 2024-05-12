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
    ElemFreeCommandQueue(commandQueue);

    ASSERT_NE(ELEM_HANDLE_NULL, commandQueue);
    ASSERT_NE(ELEM_HANDLE_NULL, commandList);
    ASSERT_FALSE(testHasLogErrors);
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
    ElemFreeCommandQueue(commandQueue);

    ASSERT_TRUE(testHasLogErrors);
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
    ElemFreeCommandQueue(commandQueue);

    ASSERT_TRUE(testHasLogErrors);
}

UTEST(CommandList, ExecuteCommandListWithoutInsertFence) 
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
    ElemFreeCommandQueue(commandQueue);

    ASSERT_FALSE(testHasLogErrors);
    ASSERT_EQ(ELEM_HANDLE_NULL, fence.CommandQueue);
}

UTEST(CommandList, ExecuteCommandListWithInsertFence) 
{
    // Arrange
    InitLog();
    auto graphicsDevice = GetSharedGraphicsDevice();
    auto commandQueue = ElemCreateCommandQueue(graphicsDevice, ElemCommandQueueType_Graphics, nullptr);
    auto commandList = ElemGetCommandList(commandQueue, nullptr);
    ElemCommitCommandList(commandList);

    // Act
    ElemExecuteCommandListOptions executeOptions = { .FenceAwaitableOnCpu = true }; 
    auto fence = ElemExecuteCommandList(commandQueue, commandList, &executeOptions);
    ElemWaitForFenceOnCpu(fence);

    // Assert
    ElemFreeCommandQueue(commandQueue);

    ASSERT_FALSE(testHasLogErrors);
    ASSERT_NE(ELEM_HANDLE_NULL, fence.CommandQueue);
}

// TODO: Test Cannot wait fence on cpu if flag was not passed
// TODO: Test mutli threading create command list
// TODO: Test also one / multi thread with multiple frame in flight (if we don't use swapchain it can grow dynamically)
// TODO: Test Fences, assign a buffer counter at each step and check the result
