#include "Elemental.h"
#include "GraphicsTests.h"
#include "utest.h"

UTEST(Rendering, RenderPassClearRenderTarget) 
{
    // Arrange
    InitLog();
    auto graphicsDevice = GetSharedGraphicsDevice();
    auto commandQueue = ElemCreateCommandQueue(graphicsDevice, ElemCommandQueueType_Graphics, nullptr);
    auto window = ElemCreateWindow(nullptr);

    // Act
    auto swapChain = ElemCreateSwapChain(commandQueue, window, nullptr, nullptr);
    
    // TODO:

    // Assert
    ElemFreeSwapChain(swapChain);
    ElemFreeWindow(window);
    ElemFreeCommandQueue(commandQueue);

    ASSERT_NE(ELEM_HANDLE_NULL, swapChain);
    ASSERT_FALSE(testHasLogErrors);
}

// TODO: Check command list type when dispatch mesh
// TODO: Multiple config for rendering
// TODO: Dispatch mesh with shader that ouput at specific locations
// TODO: TestViewports
