#include "Elemental.h"
#include "GraphicsTests.h"
#include "utest.h"

UTEST(CommandList, CreateCommandList) 
{
    // Arrange
    InitLog();
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);
    auto commandQueue = ElemCreateCommandQueue(graphicsDevice, ElemCommandQueueType_Graphics, nullptr);

    // Act
    // TODO: 
    auto commandList = ElemCreateCommandList(commandQueue, nullptr);

    // Assert
    ElemCommitCommandList(commandList);
    ElemFreeCommandQueue(commandQueue);
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_NE(ELEM_HANDLE_NULL, commandQueue);
    ASSERT_NE(ELEM_HANDLE_NULL, commandList);
    ASSERT_FALSE(testHasLogErrors);
}

UTEST(CommandList, ExecuteCommandListWithoutInsertFence) 
{
    // Arrange
    InitLog();
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);
    auto commandQueue = ElemCreateCommandQueue(graphicsDevice, ElemCommandQueueType_Graphics, nullptr);
    auto commandList = ElemCreateCommandList(commandQueue, nullptr);

    // Act
    auto fence = ElemExecuteCommandList(commandQueue, commandList, nullptr);

    // TODO: 

    // Assert
    ElemFreeCommandQueue(commandQueue);
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_FALSE(testHasLogErrors);
    ASSERT_EQ(ELEM_HANDLE_NULL, fence);
}

// TODO: Test if call exec without commit
// TODO: Test if call createcommandlist before commit last one
// TODO: Test mutli threading create command list
// TODO: Test Fences, assign a buffer counter at each step and check the result
