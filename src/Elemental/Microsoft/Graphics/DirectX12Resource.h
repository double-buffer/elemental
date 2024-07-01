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
    ElemGraphicsResourceType Type;
    DXGI_FORMAT DirectX12Format;
    D3D12_RESOURCE_FLAGS DirectX12Flags;
    uint32_t Width;
    uint32_t Height;
    uint32_t MipLevels;
    void* CpuDataPointer;
    bool IsPresentTexture;
};

struct DirectX12GraphicsResourceDataFull
{
    ElemGraphicsDevice GraphicsDevice;
};

DirectX12GraphicsHeapData* GetDirectX12GraphicsHeapData(ElemGraphicsHeap graphicsHeap);
DirectX12GraphicsHeapDataFull* GetDirectX12GraphicsHeapDataFull(ElemGraphicsHeap graphicsHeap);

DirectX12GraphicsResourceData* GetDirectX12GraphicsResourceData(ElemGraphicsResource resource);
DirectX12GraphicsResourceDataFull* GetDirectX12GraphicsResourceDataFull(ElemGraphicsResource resource);

ElemGraphicsResource CreateDirectX12GraphicsResourceFromResource(ElemGraphicsDevice graphicsDevice, ElemGraphicsResourceType type, ComPtr<ID3D12Resource> resource, bool isPresentTexture);
DXGI_FORMAT ConvertToDirectX12TextureFormat(ElemGraphicsFormat format);
            
ElemGraphicsHeap DirectX12CreateGraphicsHeap(ElemGraphicsDevice graphicsDevice, uint64_t sizeInBytes, const ElemGraphicsHeapOptions* options);
void DirectX12FreeGraphicsHeap(ElemGraphicsHeap graphicsHeap);

ElemGraphicsResourceInfo DirectX12CreateGraphicsBufferResourceInfo(ElemGraphicsDevice graphicsDevice, uint32_t sizeInBytes, ElemGraphicsResourceUsage usage, const ElemGraphicsResourceInfoOptions* options);
ElemGraphicsResourceInfo DirectX12CreateTexture2DResourceInfo(ElemGraphicsDevice graphicsDevice, uint32_t width, uint32_t height, uint32_t mipLevels, ElemGraphicsFormat format, ElemGraphicsResourceUsage usage, const ElemGraphicsResourceInfoOptions* options);

ElemGraphicsResource DirectX12CreateGraphicsResource(ElemGraphicsHeap graphicsHeap, uint64_t graphicsHeapOffset, const ElemGraphicsResourceInfo* resourceInfo);
void DirectX12FreeGraphicsResource(ElemGraphicsResource resource, const ElemFreeGraphicsResourceOptions* options);
ElemGraphicsResourceInfo DirectX12GetGraphicsResourceInfo(ElemGraphicsResource resource);
ElemDataSpan DirectX12GetGraphicsResourceDataSpan(ElemGraphicsResource resource);

ElemGraphicsResourceDescriptor DirectX12CreateGraphicsResourceDescriptor(ElemGraphicsResource resource, ElemGraphicsResourceUsage usage, const ElemGraphicsResourceDescriptorOptions* options);
ElemGraphicsResourceDescriptorInfo DirectX12GetGraphicsResourceDescriptorInfo(ElemGraphicsResourceDescriptor descriptor);
void DirectX12FreeGraphicsResourceDescriptor(ElemGraphicsResourceDescriptor descriptor, const ElemFreeGraphicsResourceDescriptorOptions* options);

void DirectX12ProcessGraphicsResourceDeleteQueue(void);

void DirectX12GraphicsResourceBarrier(ElemCommandList commandList, ElemGraphicsResourceDescriptor sourceDescriptor, ElemGraphicsResourceDescriptor destinationDescriptor, const ElemGraphicsResourceBarrierOptions* options);
