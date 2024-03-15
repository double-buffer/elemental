#include "Elemental.h"
#include "SystemFunctions.h"

bool useVulkan = false;

#ifdef _WIN32
#include "Microsoft/Graphics/Direct3D12GraphicsDevice.h"
#include "Graphics/VulkanGraphicsDevice.h"

#define DispatchGraphicsFunction(functionName, ...) \
    if (useVulkan) \
        Vulkan##functionName(__VA_ARGS__); \
    else \
        Direct3D12##functionName(__VA_ARGS__);

#define DispatchReturnGraphicsFunction(functionName, ...) \
    if (useVulkan) \
        return Vulkan##functionName(__VA_ARGS__); \
    else \
        return Direct3D12##functionName(__VA_ARGS__);
#endif

#ifdef __APPLE__
#include "Apple/Graphics/MetalGraphicsDevice.h"
#include "Graphics/VulkanGraphicsDevice.h"

#define DispatchGraphicsFunction(functionName, ...) \
    if (useVulkan) \
        Vulkan##functionName(__VA_ARGS__); \
    else \
        Metal##functionName(__VA_ARGS__);

#define DispatchReturnGraphicsFunction(functionName, ...) \
    if (useVulkan) \
        return Vulkan##functionName(__VA_ARGS__); \
    else \
        return Metal##functionName(__VA_ARGS__);
#endif

ElemAPI void ElemSetGraphicsOptions(const ElemGraphicsOptions* options)
{
    SystemAssert(options);

    useVulkan = options->PreferVulkan;

    if (options->EnableDebugLayer)
    {
        DispatchGraphicsFunction(EnableGraphicsDebugLayer);
    }
}

ElemAPI ElemGraphicsDeviceInfoList ElemGetAvailableGraphicsDevices()
{
    DispatchReturnGraphicsFunction(GetAvailableGraphicsDevices);
}

ElemAPI ElemGraphicsDevice ElemCreateGraphicsDevice(const ElemGraphicsDeviceOptions* options)
{
    DispatchReturnGraphicsFunction(CreateGraphicsDevice, options);
}

ElemAPI void ElemFreeGraphicsDevice(ElemGraphicsDevice graphicsDevice)
{
    DispatchGraphicsFunction(FreeGraphicsDevice, graphicsDevice);
}

ElemAPI ElemGraphicsDeviceInfo ElemGetGraphicsDeviceInfo(ElemGraphicsDevice graphicsDevice)
{
    DispatchReturnGraphicsFunction(GetGraphicsDeviceInfo, graphicsDevice);
}
