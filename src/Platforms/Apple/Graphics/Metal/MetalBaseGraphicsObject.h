#pragma once
#include "PreCompiledHeader.h"
#include "BaseGraphicsObject.h"

struct MetalBaseGraphicsObject : BaseGraphicsObject
{
    MetalBaseGraphicsObject(BaseGraphicsService* graphicsService, MetalGraphicsDevice* graphicsDevice) : BaseGraphicsObject(graphicsService)
    {
        GraphicsDevice = graphicsDevice;
    }

    MetalGraphicsDevice* GraphicsDevice;
};