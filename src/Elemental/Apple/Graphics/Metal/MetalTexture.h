#pragma once

#include "MetalGraphicsDevice.h"

struct MetalTexture
{
    MetalGraphicsDevice* GraphicsDevice;
    NS::SharedPtr<MTL::Texture> DeviceObject;
    bool IsPresentTexture;
};
