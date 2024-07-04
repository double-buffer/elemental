#include "Elemental.h"
#include "GraphicsCommon.h"
#include "SystemFunctions.h"

bool useVulkan = false;

ElemAPI void ElemSetGraphicsOptions(const ElemGraphicsOptions* options)
{
    SystemAssert(options);

    useVulkan = options->PreferVulkan;

    if (options->EnableDebugLayer)
    {
        DispatchGraphicsFunction(EnableGraphicsDebugLayer);
    }

    if (options->EnableGpuValidation)
    {
        DispatchGraphicsFunction(EnableGpuValidation)
    }
}

ElemAPI ElemGraphicsDeviceInfoSpan ElemGetAvailableGraphicsDevices()
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
