#include "Elemental.h"
#include "GraphicsTests.h"
#include "utest.h"

UTEST(Rendering, RenderPassClearRenderTarget) 
{
    // Arrange
    InitLog();
    auto graphicsDevice = GetSharedGraphicsDevice();
    auto commandQueue = ElemCreateCommandQueue(graphicsDevice, ElemCommandQueueType_Graphics, nullptr);
    auto application = ElemCreateApplication("TestSwapChain");
    auto window = ElemCreateWindow(application, nullptr);

    // Act
    auto swapChain = ElemCreateSwapChain(commandQueue, window, nullptr);
    
    // TODO:

    // Assert
    ElemFreeSwapChain(swapChain);
    ElemFreeWindow(window);
    ElemFreeApplication(application);
    ElemFreeCommandQueue(commandQueue);

    ASSERT_NE(ELEM_HANDLE_NULL, swapChain);
    ASSERT_FALSE(testHasLogErrors);
}

// TODO: Multiple config for rendering
// TODO: Dispatch mesh with shader that ouput at specific locations
