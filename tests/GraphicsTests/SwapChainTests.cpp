#include "Elemental.h"
#include "GraphicsTests.h"
#include "utest.h"

UTEST(SwapChain, CreateSwapChain) 
{
    // Arrange
    InitLog();
    auto graphicsDevice = GetSharedGraphicsDevice();
    auto commandQueue = ElemCreateCommandQueue(graphicsDevice, ElemCommandQueueType_Graphics, nullptr);
    auto application = ElemCreateApplication("TestSwapChain");
    auto window = ElemCreateWindow(application, nullptr);

    // Act
    auto swapChain = ElemCreateSwapChain(commandQueue, window, nullptr);

    // Assert
    ElemFreeSwapChain(swapChain);
    ElemFreeWindow(window);
    ElemFreeApplication(application);
    ElemFreeCommandQueue(commandQueue);

    ASSERT_NE(ELEM_HANDLE_NULL, swapChain);
    ASSERT_FALSE(testHasLogErrors);
}

// TODO: Resize swapchain
// TODO: Present (If present is not called during update, call it automatically but output a warning)
// TODO: GetTexture (check width, height, etc)
