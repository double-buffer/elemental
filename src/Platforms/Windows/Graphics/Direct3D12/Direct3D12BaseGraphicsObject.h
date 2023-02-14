#pragma once
#include "WindowsCommon.h"
#include "../../../Common/BaseGraphicsObject.h"
#include "Direct3D12GraphicsDevice.h"

struct Direct3D12BaseGraphicsObject : BaseGraphicsObject
{
    Direct3D12BaseGraphicsObject(BaseGraphicsService* graphicsService, Direct3D12GraphicsDevice* graphicsDevice) : BaseGraphicsObject(graphicsService)
    {
        GraphicsDevice = graphicsDevice;
    }

    Direct3D12GraphicsDevice* GraphicsDevice;
};