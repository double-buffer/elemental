#include "Elemental.h"
#include "GraphicsTests.h"
#include "utest.h"

UTEST(GraphicsDevice, GetAvailableGraphicsDevices) 
{
    // Arrange
    InitLog();
    auto application = ElemCreateApplication("TestGraphics");

    // Act
    auto graphicsDevices = ElemGetAvailableGraphicsDevices();

    // Assert
    ElemFreeApplication(application);

    auto deviceCount = 0u;

    for (uint32_t i = 0; i < graphicsDevices.Length; i++)
    {
        if ((graphicsDevices.Items[i].GraphicsApi != ElemGraphicsApi_Vulkan && testForceVulkanApi == false) || 
            (graphicsDevices.Items[i].GraphicsApi == ElemGraphicsApi_Vulkan && testForceVulkanApi == true))
        {
            deviceCount++;
        }
    }

    ASSERT_LE(1u, deviceCount);
}
