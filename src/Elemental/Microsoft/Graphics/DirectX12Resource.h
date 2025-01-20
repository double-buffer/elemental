#pragma once

#include "Elemental.h"

struct DirectX12GraphicsHeapData
{
    ComPtr<ID3D12Heap> DeviceObject;
    uint64_t SizeInBytes;
    ElemGraphicsDevice GraphicsDevice;
    D3D12_HEAP_DESC HeapDescription;
    D3D12_HEAP_TYPE HeapType;
};

// TODO: To review we don't use it!!!
struct DirectX12GraphicsHeapDataFull
{
    uint32_t reserved;
};

struct DirectX12GraphicsTextureMipCopyInfo
{
    uint32_t RowCount;
    uint64_t SourceRowSizeInBytes;
    uint32_t UploadBufferSizeInBytes;
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT PlacedFootprint;
};

// TODO: Optimize this structure because it is the most used
struct DirectX12GraphicsResourceData
{
    ComPtr<ID3D12Resource> DeviceObject;
    // TODO: We can maybe merge the two?
    D3D12_CPU_DESCRIPTOR_HANDLE RtvHandle;
    D3D12_CPU_DESCRIPTOR_HANDLE DsvHandle;
    ElemGraphicsResourceType Type;
    DXGI_FORMAT DirectX12Format;
    D3D12_RESOURCE_FLAGS DirectX12Flags;
    ElemGraphicsHeap GraphicsHeap;
    uint32_t Width;
    uint32_t Height;
    uint32_t MipLevels;
    uint32_t SubResourceOffset;
    void* CpuDataPointer;
    // TODO: Do something better than booleans here
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

ElemGraphicsResource CreateDirectX12GraphicsResourceFromResource(ElemGraphicsDevice graphicsDevice, ElemGraphicsResourceType type, ElemGraphicsHeap heap, ComPtr<ID3D12Resource> resource, bool isPresentTexture);
DXGI_FORMAT ConvertToDirectX12Format(ElemGraphicsFormat format);
bool CheckDirectX12DepthStencilFormat(ElemGraphicsFormat format);

ElemGraphicsHeap DirectX12CreateGraphicsHeap(ElemGraphicsDevice graphicsDevice, uint64_t sizeInBytes, const ElemGraphicsHeapOptions* options);
void DirectX12FreeGraphicsHeap(ElemGraphicsHeap graphicsHeap);

ElemGraphicsResourceInfo DirectX12CreateGraphicsBufferResourceInfo(ElemGraphicsDevice graphicsDevice, uint32_t sizeInBytes, ElemGraphicsResourceUsage usage, const ElemGraphicsResourceInfoOptions* options);
ElemGraphicsResourceInfo DirectX12CreateTexture2DResourceInfo(ElemGraphicsDevice graphicsDevice, uint32_t width, uint32_t height, uint32_t mipLevels, ElemGraphicsFormat format, ElemGraphicsResourceUsage usage, const ElemGraphicsResourceInfoOptions* options);

ElemGraphicsResource DirectX12CreateGraphicsResource(ElemGraphicsHeap graphicsHeap, uint64_t graphicsHeapOffset, const ElemGraphicsResourceInfo* resourceInfo);
void DirectX12FreeGraphicsResource(ElemGraphicsResource resource, const ElemFreeGraphicsResourceOptions* options);
ElemGraphicsResourceInfo DirectX12GetGraphicsResourceInfo(ElemGraphicsResource resource);

void DirectX12UploadGraphicsBufferData(ElemGraphicsResource buffer, uint32_t offset, ElemDataSpan data);
ElemDataSpan DirectX12DownloadGraphicsBufferData(ElemGraphicsResource buffer, const ElemDownloadGraphicsBufferDataOptions* options);
void DirectX12CopyDataToGraphicsResource(ElemCommandList commandList, const ElemCopyDataToGraphicsResourceParameters* parameters);

ElemGraphicsResourceDescriptor DirectX12CreateGraphicsResourceDescriptor(ElemGraphicsResource resource, ElemGraphicsResourceDescriptorUsage usage, const ElemGraphicsResourceDescriptorOptions* options);
ElemGraphicsResourceDescriptorInfo DirectX12GetGraphicsResourceDescriptorInfo(ElemGraphicsResourceDescriptor descriptor);
void DirectX12FreeGraphicsResourceDescriptor(ElemGraphicsResourceDescriptor descriptor, const ElemFreeGraphicsResourceDescriptorOptions* options);

void DirectX12ProcessGraphicsResourceDeleteQueue(ElemGraphicsDevice graphicsDevice);

void DirectX12GraphicsResourceBarrier(ElemCommandList commandList, ElemGraphicsResourceDescriptor descriptor, const ElemGraphicsResourceBarrierOptions* options);

ElemGraphicsSampler DirectX12CreateGraphicsSampler(ElemGraphicsDevice graphicsDevice, const ElemGraphicsSamplerInfo* samplerInfo);
ElemGraphicsSamplerInfo DirectX12GetGraphicsSamplerInfo(ElemGraphicsSampler sampler);
void DirectX12FreeGraphicsSampler(ElemGraphicsSampler sampler, const ElemFreeGraphicsSamplerOptions* options);

ElemRaytracingAllocationInfo DirectX12GetRaytracingBlasAllocationInfo(ElemGraphicsDevice graphicsDevice, const ElemRaytracingBlasParameters* parameters);
ElemRaytracingAllocationInfo DirectX12GetRaytracingTlasAllocationInfo(ElemGraphicsDevice graphicsDevice, const ElemRaytracingTlasParameters* parameters);
ElemGraphicsResourceAllocationInfo DirectX12GetRaytracingTlasInstanceAllocationInfo(ElemGraphicsDevice graphicsDevice, uint32_t instanceCount);
ElemDataSpan DirectX12EncodeRaytracingTlasInstances(ElemRaytracingTlasInstanceSpan instances);

ElemGraphicsResource DirectX12CreateRaytracingAccelerationStructureResource(ElemGraphicsDevice graphicsDevice, ElemGraphicsResource storageBuffer, const ElemRaytracingAccelerationStructureOptions* options);

void DirectX12BuildRaytracingBlas(ElemCommandList commandList, ElemGraphicsResource accelerationStructure, ElemGraphicsResource scratchBuffer, const ElemRaytracingBlasParameters* parameters, const ElemRaytracingBuildOptions* options);
void DirectX12BuildRaytracingTlas(ElemCommandList commandList, ElemGraphicsResource accelerationStructure, ElemGraphicsResource scratchBuffer, const ElemRaytracingTlasParameters* parameters, const ElemRaytracingBuildOptions* options);
