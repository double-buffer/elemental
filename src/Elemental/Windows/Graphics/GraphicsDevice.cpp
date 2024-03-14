#include "Elemental.h"
#include "SystemFunctions.h"
#include "Direct3D12/Direct3D12GraphicsDevice.h"

ElemAPI void ElemEnableGraphicsDebugLayer()
{
    Direct3D12EnableGraphicsDebugLayer();
}

ElemAPI ElemGraphicsDeviceInfoList ElemGetAvailableGraphicsDevices()
{
    return Direct3D12GetAvailableGraphicsDevices();
}

ElemAPI ElemGraphicsDevice ElemCreateGraphicsDevice(const ElemGraphicsDeviceOptions* options)
{
    ElemGraphicsDeviceOptions localOptions = {};

    if (options)
    {
        localOptions = *options;
    }

    auto graphicsDevices = ElemGetAvailableGraphicsDevices();
    SystemAssert(graphicsDevices.Length);

    auto selectedDevice = graphicsDevices.Items[0];

    if (localOptions.DeviceId != 0)
    {
        for (uint32_t i = 0; i < graphicsDevices.Length; i++)
        {
            if (graphicsDevices.Items[i].DeviceId == options->DeviceId)
            {
                selectedDevice = graphicsDevices.Items[i];
                break;
            }
        }
    }
    
    localOptions.DeviceId = selectedDevice.DeviceId;
 
    if (selectedDevice.GraphicsApi == ElemGraphicsApi_Vulkan)
    {
        return {};//VulkanCreateGraphicsDevice(options);
    }
    else
    {
        return Direct3D12CreateGraphicsDevice(&localOptions);
    }
}

ElemAPI void ElemFreeGraphicsDevice(ElemGraphicsDevice graphicsDevice)
{
    Direct3D12FreeGraphicsDevice(graphicsDevice);
}

ElemAPI ElemGraphicsDeviceInfo ElemGetGraphicsDeviceInfo(ElemGraphicsDevice graphicsDevice)
{
    return Direct3D12GetGraphicsDeviceInfo(graphicsDevice);
}
