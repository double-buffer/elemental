#pragma once

#ifdef _WINDOWS
#include "WindowsCommon.h"
#endif

#include "../BaseGraphicsService.h"
#include "../BaseGraphicsObject.h"
#include "../StringConverters.h"

class VulkanGraphicsService : BaseGraphicsService
{
public:
    void* CreateGraphicsDevice(GraphicsDeviceOptions options) override;
    void FreeGraphicsDevice(void *graphicsDevicePointer) override;
    GraphicsDeviceInfo GetGraphicsDeviceInfo(void *graphicsDevicePointer) override;
};