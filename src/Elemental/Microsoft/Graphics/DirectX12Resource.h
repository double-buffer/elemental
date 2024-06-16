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

struct DirectX12GraphicsResourceData
{
    ComPtr<ID3D12Resource> DeviceObject;
    DXGI_FORMAT DirectX12Format;
    uint32_t Width;
    uint32_t Height;
    uint32_t MipLevels;
    bool IsPresentTexture;
    D3D12_CPU_DESCRIPTOR_HANDLE RtvDescriptor;
};

struct DirectX12GraphicsResourceDataFull
{
    ElemGraphicsDevice GraphicsDevice;
};

DirectX12GraphicsHeapData* GetDirectX12GraphicsHeapData(ElemGraphicsHeap graphicsHeap);
DirectX12GraphicsHeapDataFull* GetDirectX12GraphicsHeapDataFull(ElemGraphicsHeap graphicsHeap);

DirectX12GraphicsResourceData* GetDirectX12GraphicsResourceData(ElemGraphicsResource texture);
DirectX12GraphicsResourceDataFull* GetDirectX12GraphicsResourceDataFull(ElemGraphicsResource texture);

ElemGraphicsResource CreateDirectX12TextureFromResource(ElemGraphicsDevice graphicsDevice, ComPtr<ID3D12Resource> resource, bool isPresentTexture);
DXGI_FORMAT ConvertToDirectX12TextureFormat(ElemGraphicsFormat format);
            
ElemGraphicsHeap DirectX12CreateGraphicsHeap(ElemGraphicsDevice graphicsDevice, uint64_t sizeInBytes, const ElemGraphicsHeapOptions* options);
void DirectX12FreeGraphicsHeap(ElemGraphicsHeap graphicsHeap);

ElemGraphicsResource DirectX12CreateGraphicsResource(ElemGraphicsHeap graphicsHeap, uint64_t graphicsHeapOffset, const ElemGraphicsResourceInfo* resourceInfo);
void DirectX12FreeGraphicsResource(ElemGraphicsResource resource);

ElemShaderDescriptor DirectX12CreateTextureShaderDescriptor(ElemGraphicsResource texture, const ElemTextureShaderDescriptorOptions* options);
void DirectX12FreeShaderDescriptor(ElemShaderDescriptor shaderDescriptor);
