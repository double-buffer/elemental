#pragma once

#include "Elemental.h"

struct DirectX12TextureData
{
    ComPtr<ID3D12Resource> DeviceObject;
    D3D12_RESOURCE_DESC ResourceDescription;
    bool IsPresentTexture;
    D3D12_CPU_DESCRIPTOR_HANDLE RtvDescriptor;
};

struct DirectX12TextureDataFull
{
    ElemGraphicsDevice GraphicsDevice;
};

DirectX12TextureData* GetDirectX12TextureData(ElemTexture texture);
DirectX12TextureDataFull* GetDirectX12TextureDataFull(ElemTexture texture);

ElemTexture CreateDirectX12TextureFromResource(ElemGraphicsDevice graphicsDevice, ComPtr<ID3D12Resource> resource, bool isPresentTexture);
DXGI_FORMAT ConvertToDirectX12TextureFormat(ElemTextureFormat format);
            
void DirectX12FreeTexture(ElemTexture texture);
