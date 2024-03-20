#include "Elemental.h"
#include "GraphicsTests.h"
#include "utest.h"

UTEST(CommandList, CreateCommandList) 
{
    // Arrange
    InitLog();
    auto graphicsDevice = GetSharedGraphicsDevice();
    auto commandQueue = ElemCreateCommandQueue(graphicsDevice, ElemCommandQueueType_Graphics, nullptr);

    // Act
    auto commandList = ElemCreateCommandList(commandQueue, nullptr);

    // Assert
    ElemCommitCommandList(commandList);
    ElemFreeCommandQueue(commandQueue);

    ASSERT_NE(ELEM_HANDLE_NULL, commandQueue);
    ASSERT_NE(ELEM_HANDLE_NULL, commandList);
    ASSERT_FALSE(testHasLogErrors);
}

UTEST(CommandList, CreateCommandListWithoutPreviousCommitted) 
{
    // Arrange
    InitLog();
    auto graphicsDevice = GetSharedGraphicsDevice();
    auto commandQueue = ElemCreateCommandQueue(graphicsDevice, ElemCommandQueueType_Graphics, nullptr);
    ElemCreateCommandList(commandQueue, nullptr);

    // Act
    ElemCreateCommandList(commandQueue, nullptr);

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
    auto commandList = ElemCreateCommandList(commandQueue, nullptr);

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
    auto commandList = ElemCreateCommandList(commandQueue, nullptr);
    ElemCommitCommandList(commandList);

    // Act
    auto fence = ElemExecuteCommandList(commandQueue, commandList, nullptr);

    // Assert
    ElemFreeCommandQueue(commandQueue);

    ASSERT_FALSE(testHasLogErrors);
    ASSERT_EQ(ELEM_HANDLE_NULL, fence);
}

UTEST(CommandList, ExecuteCommandListWithInsertFence) 
{
    // Arrange
    InitLog();
    auto graphicsDevice = GetSharedGraphicsDevice();
    auto commandQueue = ElemCreateCommandQueue(graphicsDevice, ElemCommandQueueType_Graphics, nullptr);
    auto commandList = ElemCreateCommandList(commandQueue, nullptr);
    ElemCommitCommandList(commandList);

    // Act
    ElemExecuteCommandListOptions executeOptions = { .InsertFence = true }; 
    auto fence = ElemExecuteCommandList(commandQueue, commandList, &executeOptions);
    ElemWaitForFenceOnCpu(fence);

    // Assert
    ElemFreeCommandQueue(commandQueue);

    ASSERT_FALSE(testHasLogErrors);
    ASSERT_NE(ELEM_HANDLE_NULL, fence);
}

// TODO: Test mutli threading create command list
// TODO: Test Fences, assign a buffer counter at each step and check the result
