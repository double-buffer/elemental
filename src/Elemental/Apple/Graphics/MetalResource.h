#pragma once

#include "Elemental.h"

struct MetalGraphicsHeapData
{
    NS::SharedPtr<MTL::Heap> DeviceObject;
    uint64_t SizeInBytes;
    ElemGraphicsDevice GraphicsDevice;
};

struct MetalGraphicsHeapDataFull
{
    uint32_t Reserved;
};

struct MetalResourceData
{
    NS::SharedPtr<MTL::Texture> DeviceObject;
    uint32_t Width;
    uint32_t Height;
    bool IsPresentTexture;
};

struct MetalResourceDataFull
{
    ElemGraphicsDevice GraphicsDevice;
};

MetalGraphicsHeapData* GetMetalGraphicsHeapData(ElemGraphicsHeap graphicsHeap);
MetalGraphicsHeapDataFull* GetMetalGraphicsHeapDataFull(ElemGraphicsHeap graphicsHeap);

MetalResourceData* GetMetalResourceData(ElemGraphicsResource resource);
MetalResourceDataFull* GetMetalResourceDataFull(ElemGraphicsResource resource);

ElemGraphicsHeap MetalCreateGraphicsHeap(ElemGraphicsDevice graphicsDevice, uint64_t sizeInBytes, const ElemGraphicsHeapOptions* options);
void MetalFreeGraphicsHeap(ElemGraphicsHeap graphicsHeap);

ElemGraphicsResource MetalCreateGraphicsResource(ElemGraphicsHeap graphicsHeap, uint64_t graphicsHeapOffset, const ElemGraphicsResourceInfo* resourceInfo);
void MetalFreeGraphicsResource(ElemGraphicsResource resource);

ElemShaderDescriptor MetalCreateTextureShaderDescriptor(ElemGraphicsResource texture, const ElemTextureShaderDescriptorOptions* options);
void MetalFreeShaderDescriptor(ElemShaderDescriptor shaderDescriptor);

ElemGraphicsResource CreateMetalTextureFromResource(ElemGraphicsDevice graphicsDevice, NS::SharedPtr<MTL::Texture> resource, bool isPresentTexture);
