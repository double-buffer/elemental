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

UTEST(GraphicsDevice, CreateGraphicsDevice) 
{
    // Arrange
    InitLog();
    auto application = ElemCreateApplication("TestGraphics");
    auto graphicsDevices = ElemGetAvailableGraphicsDevices();
    ElemGraphicsDeviceInfo graphicsDeviceInfo = {};

    for (uint32_t i = 0; i < graphicsDevices.Length; i++)
    {
        if ((graphicsDevices.Items[i].GraphicsApi != ElemGraphicsApi_Vulkan && testForceVulkanApi == false) || 
            (graphicsDevices.Items[i].GraphicsApi == ElemGraphicsApi_Vulkan && testForceVulkanApi == true))
        {
            graphicsDeviceInfo = graphicsDevices.Items[i];
            break;
        }
    }

    ASSERT_NE(0u, graphicsDeviceInfo.DeviceId);

    char deviceName[255];
    strcpy(deviceName, graphicsDeviceInfo.DeviceName);
        
    // Act
    ElemGraphicsDeviceOptions options = { .DeviceId = graphicsDeviceInfo.DeviceId };
    auto graphicsDevice = ElemCreateGraphicsDevice(&options); 

    // Assert
    auto resultDeviceInfo = ElemGetGraphicsDeviceInfo(graphicsDevice);
    
    ElemFreeGraphicsDevice(graphicsDevice);
    ElemFreeApplication(application);

    ASSERT_EQ(graphicsDeviceInfo.DeviceId, resultDeviceInfo.DeviceId); 
    ASSERT_STREQ(deviceName, resultDeviceInfo.DeviceName); 
    ASSERT_EQ(graphicsDeviceInfo.GraphicsApi, resultDeviceInfo.GraphicsApi); 
    ASSERT_EQ(graphicsDeviceInfo.AvailableMemory, resultDeviceInfo.AvailableMemory); 
}
