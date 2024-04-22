#include "DirectX12Texture.h"
#include "DirectX12GraphicsDevice.h"
#include "SystemDataPool.h"
#include "SystemFunctions.h"
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

DXGI_FORMAT ConvertDirectX12FormatToSrgbIfNeeded(DXGI_FORMAT format)
{
    switch (format) 
    {
        case DXGI_FORMAT_B8G8R8A8_UNORM:
            return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;

        default:
            return format;
    }
}

ElemTexture CreateDirectX12TextureFromResource(ElemGraphicsDevice graphicsDevice, ComPtr<ID3D12Resource> resource, bool isPresentTexture)
{
    InitDirectX12TextureMemory();

    auto graphicsDeviceData = GetDirectX12GraphicsDeviceData(graphicsDevice);
    SystemAssert(graphicsDeviceData);

    auto graphicsDeviceDataFull = GetDirectX12GraphicsDeviceDataFull(graphicsDevice);
    SystemAssert(graphicsDeviceDataFull);

    auto resourceDesc = resource->GetDesc();
    D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptor = {};

    if (resourceDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET)
    {
        SystemLogDebugMessage(ElemLogMessageCategory_Graphics, "Creating RTV...");

        rtvDescriptor = CreateDirectX12DescriptorHandle(graphicsDeviceDataFull->RTVDescriptorHeap);

        D3D12_RENDER_TARGET_VIEW_DESC renderTargetViewDesc = 
        {
            .Format = ConvertDirectX12FormatToSrgbIfNeeded(resourceDesc.Format),
            .ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D,
        };
 
        graphicsDeviceData->Device->CreateRenderTargetView(resource.Get(), &renderTargetViewDesc, rtvDescriptor);
    }

    auto handle = SystemAddDataPoolItem(directX12TexturePool, {
        .DeviceObject = resource,
        .ResourceDescription = resourceDesc,
        .IsPresentTexture = isPresentTexture,
        .RtvDescriptor = rtvDescriptor
    }); 

    SystemAddDataPoolItemFull(directX12TexturePool, handle, {
        .GraphicsDevice = graphicsDevice
    });

    return handle;
}

DXGI_FORMAT ConvertToDirectX12TextureFormat(ElemTextureFormat format)
{
    switch (format) 
    {
        case ElemTextureFormat_B8G8R8A8_SRGB:
            return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
    }
}

// TODO: Functions to create additional optional Descriptors

void DirectX12FreeTexture(ElemTexture texture)
{
    // TODO: Do a kind a deferred texture delete so we don't crash if the resource is still in use
    SystemAssert(texture != ELEM_HANDLE_NULL);

    auto textureData = GetDirectX12TextureData(texture);
    SystemAssert(textureData);

    auto textureDataFull = GetDirectX12TextureDataFull(texture);
    SystemAssert(textureDataFull);

    auto graphicsDeviceDataFull = GetDirectX12GraphicsDeviceDataFull(textureDataFull->GraphicsDevice);
    SystemAssert(graphicsDeviceDataFull);

    // TODO: Remove descriptor handles
    if (textureData->RtvDescriptor.ptr)
    {
        FreeDirectX12DescriptorHandle(graphicsDeviceDataFull->RTVDescriptorHeap, textureData->RtvDescriptor);
        textureData->RtvDescriptor.ptr = 0;
    }

    if (textureData->DeviceObject)
    {
        textureData->DeviceObject.Reset();
    }

    SystemRemoveDataPoolItem(directX12TexturePool, texture);
}
