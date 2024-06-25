#include "Elemental.h"
#include "GraphicsTests.h"
#include "utest.h"

UTEST(GraphicsDevice, GetAvailableGraphicsDevices) 
{
    // Act
    auto graphicsDevices = ElemGetAvailableGraphicsDevices();

    // Assert
    auto deviceCount = 0u;

    for (uint32_t i = 0; i < graphicsDevices.Length; i++)
    {
        if ((graphicsDevices.Items[i].GraphicsApi != ElemGraphicsApi_Vulkan && testForceVulkanApi == false) || 
            (graphicsDevices.Items[i].GraphicsApi == ElemGraphicsApi_Vulkan && testForceVulkanApi == true))
        {
            deviceCount++;
        }
    }

    ASSERT_LOG_NOERROR();
    ASSERT_GE(deviceCount, 1u);
}

UTEST(GraphicsDevice, CreateGraphicsDevice) 
{
    // Arrange
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
    ASSERT_NE_MSG(graphicsDevice, ELEM_HANDLE_NULL, "Handle should not be null.");

    // Assert
    auto resultDeviceInfo = ElemGetGraphicsDeviceInfo(graphicsDevice);

    ASSERT_LOG_NOERROR();
    ASSERT_EQ(resultDeviceInfo.DeviceId, graphicsDeviceInfo.DeviceId); 
    ASSERT_STREQ(resultDeviceInfo.DeviceName, deviceName); 
    ASSERT_EQ(resultDeviceInfo.GraphicsApi, graphicsDeviceInfo.GraphicsApi); 
    ASSERT_EQ(resultDeviceInfo.AvailableMemory, graphicsDeviceInfo.AvailableMemory); 
    
    ElemFreeGraphicsDevice(graphicsDevice);
}

// TODO: Test Shader Resource Descriptors 
