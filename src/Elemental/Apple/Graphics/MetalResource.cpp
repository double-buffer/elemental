#include "MetalTexture.h"
#include "MetalGraphicsDevice.h"
#include "SystemDataPool.h"
#include "SystemFunctions.h"
#include "SystemMemory.h"

#define METAL_MAX_GRAPHICSHEAP 32
#define METAL_MAX_TEXTURES UINT16_MAX

SystemDataPool<MetalGraphicsHeapData, MetalGraphicsHeapDataFull> metalGraphicsHeapPool;
SystemDataPool<MetalTextureData, MetalTextureDataFull> metalTexturePool;

void InitMetalTextureMemory()
{
    if (!metalGraphicsHeapPool.Storage)
    {
        metalGraphicsHeapPool = SystemCreateDataPool<MetalGraphicsHeapData, MetalGraphicsHeapDataFull>(MetalGraphicsMemoryArena, METAL_MAX_GRAPHICSHEAP);
        metalTexturePool = SystemCreateDataPool<MetalTextureData, MetalTextureDataFull>(MetalGraphicsMemoryArena, METAL_MAX_TEXTURES);
    }
}

MetalGraphicsHeapData* GetMetalGraphicsHeapData(ElemGraphicsHeap graphicsHeap)
{
    return SystemGetDataPoolItem(metalGraphicsHeapPool, graphicsHeap);
}

MetalGraphicsHeapDataFull* GetMetalGraphicsHeapDataFull(ElemGraphicsHeap graphicsHeap)
{
    return SystemGetDataPoolItemFull(metalGraphicsHeapPool, graphicsHeap);
}

MetalTextureData* GetMetalTextureData(ElemTexture texture)
{
    return SystemGetDataPoolItem(metalTexturePool, texture);
}

MetalTextureDataFull* GetMetalTextureDataFull(ElemTexture texture)
{
    return SystemGetDataPoolItemFull(metalTexturePool, texture);
}

ElemTexture CreateMetalTextureFromResource(ElemGraphicsDevice graphicsDevice, NS::SharedPtr<MTL::Texture> resource, bool isPresentTexture)
{
    InitMetalTextureMemory();

    auto handle = SystemAddDataPoolItem(metalTexturePool, {
        .DeviceObject = resource,
        .Width = (uint32_t)resource->width(),
        .Height = (uint32_t)resource->height(),
        .IsPresentTexture = isPresentTexture,
    }); 

    SystemAddDataPoolItemFull(metalTexturePool, handle, {
        .GraphicsDevice = graphicsDevice
    });

    return handle;
}

ElemGraphicsHeap MetalCreateGraphicsHeap(ElemGraphicsDevice graphicsDevice, uint64_t sizeInBytes, const ElemGraphicsHeapOptions* options)
{
    return {};
}

void MetalFreeGraphicsHeap(ElemGraphicsHeap graphicsHeap)
{
}

void MetalBindGraphicsHeap(ElemCommandList commandList, ElemGraphicsHeap graphicsHeap)
{
}

ElemTexture MetalCreateTexture(ElemGraphicsHeap graphicsHeap, uint64_t graphicsHeapOffset, const ElemTextureParameters* parameters)
{
    return {};
}

void MetalFreeTexture(ElemTexture texture)
{
    auto textureData = GetMetalTextureData(texture);
    SystemAssert(textureData);

    SystemRemoveDataPoolItem(metalTexturePool, texture);
}

ElemShaderDescriptor MetalCreateTextureShaderDescriptor(ElemTexture texture, const ElemTextureShaderDescriptorOptions* options)
{
    return 0;
}

void MetalFreeShaderDescriptor(ElemShaderDescriptor shaderDescriptor)
{
}
