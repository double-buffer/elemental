#include "DirectX12Texture.h"
#include "DirectX12GraphicsDevice.h"
#include "SystemDataPool.h"
#include "SystemMemory.h"

#define DIRECTX12_MAX_TEXTURES UINT16_MAX

SystemDataPool<DirectX12TextureData, DirectX12TextureDataFull> directX12TexturePool;

void InitDirectX12TextureMemory()
{
    if (!directX12TexturePool.Storage)
    {
        directX12TexturePool = SystemCreateDataPool<DirectX12TextureData, DirectX12TextureDataFull>(DirectX12MemoryArena, DIRECTX12_MAX_TEXTURES);
    }
}

DirectX12TextureData* GetDirectX12TextureData(ElemTexture texture)
{
    return SystemGetDataPoolItem(directX12TexturePool, texture);
}

DirectX12TextureDataFull* GetDirectX12TextureDataFull(ElemTexture texture)
{
    return SystemGetDataPoolItemFull(directX12TexturePool, texture);
}
