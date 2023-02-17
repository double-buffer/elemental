#pragma once
#include "WindowsCommon.h"
#include "Direct3D12BaseGraphicsObject.h"

struct Direct3D12Texture : Direct3D12BaseGraphicsObject
{
    Direct3D12Texture(BaseGraphicsService* graphicsService, Direct3D12GraphicsDevice* graphicsDevice) : Direct3D12BaseGraphicsObject(graphicsService, graphicsDevice)
    {
    }
};