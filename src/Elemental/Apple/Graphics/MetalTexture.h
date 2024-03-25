#pragma once

#include "Elemental.h"

struct MetalTextureData
{
    NS::SharedPtr<MTL::Texture> DeviceObject;
    uint32_t Width;
    uint32_t Height;
    bool IsPresentTexture;
};

struct MetalTextureDataFull
{
    ElemGraphicsDevice GraphicsDevice;
};

MetalTextureData* GetMetalTextureData(ElemTexture texture);
MetalTextureDataFull* GetMetalTextureDataFull(ElemTexture texture);

void MetalFreeTexture(ElemTexture texture);

ElemTexture CreateMetalTextureFromResource(ElemGraphicsDevice graphicsDevice, NS::SharedPtr<MTL::Texture> resource, bool isPresentTexture);
