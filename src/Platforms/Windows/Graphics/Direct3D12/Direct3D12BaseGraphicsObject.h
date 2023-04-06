#pragma once
#include "WindowsCommon.h"
#include "BaseGraphicsObject.h"

struct Direct3D12BaseGraphicsObject : BaseGraphicsObject
{
    Direct3D12BaseGraphicsObject(BaseGraphicsService* graphicsService, Direct3D12GraphicsDevice* graphicsDevice) : BaseGraphicsObject(graphicsService)
    {
        GraphicsDevice = graphicsDevice;
    }

    Direct3D12GraphicsDevice* GraphicsDevice;
};