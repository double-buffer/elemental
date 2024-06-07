#pragma once

#include "Elemental.h"

struct MetalGraphicsHeapData
{
};

struct MetalGraphicsHeapDataFull
{
    ElemGraphicsDevice GraphicsDevice;
};

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

MetalGraphicsHeapData* GetMetalGraphicsHeapData(ElemGraphicsHeap graphicsHeap);
MetalGraphicsHeapDataFull* GetMetalGraphicsHeapDataFull(ElemGraphicsHeap graphicsHeap);

MetalTextureData* GetMetalTextureData(ElemTexture texture);
MetalTextureDataFull* GetMetalTextureDataFull(ElemTexture texture);

ElemGraphicsHeap MetalCreateGraphicsHeap(ElemGraphicsDevice graphicsDevice, uint64_t sizeInBytes, const ElemGraphicsHeapOptions* options);
void MetalFreeGraphicsHeap(ElemGraphicsHeap graphicsHeap);

ElemTexture MetalCreateTexture(ElemGraphicsHeap graphicsHeap, uint64_t graphicsHeapOffset, const ElemTextureParameters* parameters);
void MetalFreeTexture(ElemTexture texture);

ElemShaderDescriptor MetalCreateTextureShaderDescriptor(ElemTexture texture, const ElemTextureShaderDescriptorOptions* options);
void MetalFreeShaderDescriptor(ElemShaderDescriptor shaderDescriptor);

ElemTexture CreateMetalTextureFromResource(ElemGraphicsDevice graphicsDevice, NS::SharedPtr<MTL::Texture> resource, bool isPresentTexture);
