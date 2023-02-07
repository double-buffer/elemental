#pragma once

#ifdef _WINDOWS
#include "WindowsCommon.h"
#endif

#include "../BaseGraphicsService.h"
#include "../BaseGraphicsObject.h"
#include "../StringConverters.h"

#define VK_USE_PLATFORM_WIN32_KHR
#define VOLK_IMPLEMENTATION
#include "Volk/volk.h"

class VulkanGraphicsService : BaseGraphicsService
{
public:
    VulkanGraphicsService(GraphicsServiceOptions options);

    void GetAvailableGraphicsDevices(GraphicsDeviceInfo* graphicsDevices, int* count) override;
    void* CreateGraphicsDevice(GraphicsDeviceOptions options) override;
    void FreeGraphicsDevice(void *graphicsDevicePointer) override;
    GraphicsDeviceInfo GetGraphicsDeviceInfo(void *graphicsDevicePointer) override;

private:
    void InitSdk(bool enableDebugDiagnostics);
    GraphicsDeviceInfo ConstructGraphicsDeviceInfo(int adapterDescription);
};