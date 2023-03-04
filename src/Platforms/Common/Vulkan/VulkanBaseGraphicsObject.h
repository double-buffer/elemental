#pragma once
#ifdef _WINDOWS
#include "WindowsCommon.h"
#endif
#include "Volk/volk.h"
#include "../BaseGraphicsObject.h"

struct VulkanBaseGraphicsObject : BaseGraphicsObject
{
    VulkanBaseGraphicsObject(BaseGraphicsService* graphicsService, VulkanGraphicsDevice* graphicsDevice) : BaseGraphicsObject(graphicsService)
    {
        GraphicsDevice = graphicsDevice;
    }

    VulkanGraphicsDevice* GraphicsDevice;
};