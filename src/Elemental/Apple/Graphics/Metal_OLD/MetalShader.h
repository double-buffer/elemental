#pragma once

#include "MetalGraphicsDevice.h"

struct MetalShader
{
    MetalGraphicsDevice* GraphicsDevice;
    NS::SharedPtr<MTL::Function> AmplificationShader;
    MTL::Size AmplificationThreadCount;
    NS::SharedPtr<MTL::Function> MeshShader;
    MTL::Size MeshThreadCount;

    NS::SharedPtr<MTL::Function> PixelShader;
};
