#include "MetalTexture.h"
#include "MetalGraphicsDevice.h"
#include "SystemDataPool.h"
#include "SystemFunctions.h"
#include "SystemMemory.h"

#define DIRECTX12_MAX_TEXTURES UINT16_MAX

SystemDataPool<MetalTextureData, MetalTextureDataFull> metalTexturePool;

void InitMetalTextureMemory()
{
    if (!metalTexturePool.Storage)
    {
        metalTexturePool = SystemCreateDataPool<MetalTextureData, MetalTextureDataFull>(MetalGraphicsMemoryArena, DIRECTX12_MAX_TEXTURES);
    }
}

MetalTextureData* GetMetalTextureData(ElemTexture texture)
{
    return SystemGetDataPoolItem(metalTexturePool, texture);
}

MetalTextureDataFull* GetMetalTextureDataFull(ElemTexture texture)
{
    return SystemGetDataPoolItemFull(metalTexturePool, texture);
}

void MetalFreeTexture(ElemTexture texture)
{
    auto textureData = GetMetalTextureData(texture);
    SystemAssert(textureData);

    SystemRemoveDataPoolItem(metalTexturePool, texture);
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

