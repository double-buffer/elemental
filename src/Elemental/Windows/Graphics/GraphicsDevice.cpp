#include "Elemental.h"
#include "SystemFunctions.h"
#include "SystemMemory.h"
#include "Direct3D12/Direct3D12GraphicsDevice.h"
#include "Vulkan/VulkanGraphicsDevice.h"

#define GRAPHICS_MAXDEVICES 16

ElemAPI void ElemEnableGraphicsDebugLayer()
{
    Direct3D12EnableGraphicsDebugLayer();
    VulkanEnableGraphicsDebugLayer();
}

ElemAPI ElemGraphicsDeviceInfoList ElemGetAvailableGraphicsDevices()
{
    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto result = SystemPushArray<ElemGraphicsDeviceInfo>(stackMemoryArena, GRAPHICS_MAXDEVICES);
    
    auto direct3DDevices = Direct3D12GetAvailableGraphicsDevices();
    auto direct3DDeviceCount = direct3DDevices.Length;
    SystemCopyBuffer(result, ReadOnlySpan<ElemGraphicsDeviceInfo>(direct3DDevices.Items, direct3DDeviceCount));

    auto vulkanDevices = VulkanGetAvailableGraphicsDevices();
    auto vulkanDeviceCount = vulkanDevices.Length;
    SystemCopyBuffer(result.Slice(direct3DDeviceCount), ReadOnlySpan<ElemGraphicsDeviceInfo>(vulkanDevices.Items, vulkanDeviceCount));

    return 
    {
        .Items = result.Pointer,
        .Length = direct3DDeviceCount + vulkanDeviceCount
    };
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
        return VulkanCreateGraphicsDevice(&localOptions);
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
