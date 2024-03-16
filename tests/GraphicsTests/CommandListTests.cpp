#include "Elemental.h"
#include "GraphicsTests.h"
#include "utest.h"

UTEST(CommandList, CreateCommandList) 
{
    // Arrange
    InitLog();
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);
    auto commandQueue = ElemCreateGraphicsCommandQueue(graphicsDevice, ElemGraphicsCommandQueueType_Graphics, nullptr);

    // Act
    // TODO: 

    // Assert
    ElemFreeGraphicsCommandQueue(commandQueue);
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_NE(ELEM_HANDLE_NULL, commandQueue);
    ASSERT_FALSE(testHasLogErrors);
}
