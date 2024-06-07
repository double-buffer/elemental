#pragma once

#include "Elemental.h"

struct DirectX12GraphicsHeapData
{
    ComPtr<ID3D12Heap> DeviceObject;
    uint64_t SizeInBytes;
    ElemGraphicsDevice GraphicsDevice;
};

struct DirectX12GraphicsHeapDataFull
{
    D3D12_HEAP_DESC HeapDescription;
};

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

DirectX12GraphicsHeapData* GetDirectX12GraphicsHeapData(ElemGraphicsHeap graphicsHeap);
DirectX12GraphicsHeapDataFull* GetDirectX12GraphicsHeapDataFull(ElemGraphicsHeap graphicsHeap);

DirectX12TextureData* GetDirectX12TextureData(ElemTexture texture);
DirectX12TextureDataFull* GetDirectX12TextureDataFull(ElemTexture texture);

ElemTexture CreateDirectX12TextureFromResource(ElemGraphicsDevice graphicsDevice, ComPtr<ID3D12Resource> resource, bool isPresentTexture);
DXGI_FORMAT ConvertToDirectX12TextureFormat(ElemTextureFormat format);
            
ElemGraphicsHeap DirectX12CreateGraphicsHeap(ElemGraphicsDevice graphicsDevice, uint64_t sizeInBytes, const ElemGraphicsHeapOptions* options);
void DirectX12FreeGraphicsHeap(ElemGraphicsHeap graphicsHeap);

ElemTexture DirectX12CreateTexture(ElemGraphicsHeap graphicsHeap, uint64_t graphicsHeapOffset, const ElemTextureParameters* parameters);
void DirectX12FreeTexture(ElemTexture texture);

ElemShaderDescriptor DirectX12CreateTextureShaderDescriptor(ElemTexture texture, const ElemTextureShaderDescriptorOptions* options);
void DirectX12FreeShaderDescriptor(ElemShaderDescriptor shaderDescriptor);
