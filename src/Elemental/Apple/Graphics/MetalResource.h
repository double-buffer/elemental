#pragma once

#include "Elemental.h"

struct MetalGraphicsHeapData
{
    NS::SharedPtr<MTL::Heap> DeviceObject;
    uint64_t SizeInBytes;
    ElemGraphicsDevice GraphicsDevice;
    ElemGraphicsHeapType HeapType;
};

struct MetalGraphicsHeapDataFull
{
    uint32_t Reserved;
};

struct MetalResourceData
{
    NS::SharedPtr<MTL::Resource> DeviceObject;
    ElemGraphicsHeap GraphicsHeap;
    ElemGraphicsResourceType Type;
    uint32_t Width;
    uint32_t Height;
    uint32_t MipLevels;
    ElemGraphicsFormat Format;
    ElemGraphicsResourceUsage Usage;
    bool IsPresentTexture;
};

struct MetalResourceDataFull
{
    ElemGraphicsDevice GraphicsDevice;
};

struct MetalGraphicsSamplerInfo
{
    NS::SharedPtr<MTL::SamplerState> MetalSampler;
    ElemGraphicsSamplerInfo SamplerInfo;
};

MetalGraphicsHeapData* GetMetalGraphicsHeapData(ElemGraphicsHeap graphicsHeap);
MetalGraphicsHeapDataFull* GetMetalGraphicsHeapDataFull(ElemGraphicsHeap graphicsHeap);

MetalResourceData* GetMetalResourceData(ElemGraphicsResource resource);
MetalResourceDataFull* GetMetalResourceDataFull(ElemGraphicsResource resource);

MTL::PixelFormat ConvertToMetalResourceFormat(ElemGraphicsFormat format);

ElemGraphicsHeap MetalCreateGraphicsHeap(ElemGraphicsDevice graphicsDevice, uint64_t sizeInBytes, const ElemGraphicsHeapOptions* options);
void MetalFreeGraphicsHeap(ElemGraphicsHeap graphicsHeap);

ElemGraphicsResourceInfo MetalCreateGraphicsBufferResourceInfo(ElemGraphicsDevice graphicsDevice, uint32_t sizeInBytes, ElemGraphicsResourceUsage usage, const ElemGraphicsResourceInfoOptions* options);
ElemGraphicsResourceInfo MetalCreateTexture2DResourceInfo(ElemGraphicsDevice graphicsDevice, uint32_t width, uint32_t height, uint32_t mipLevels, ElemGraphicsFormat format, ElemGraphicsResourceUsage usage, const ElemGraphicsResourceInfoOptions* options);

ElemGraphicsResource MetalCreateGraphicsResource(ElemGraphicsHeap graphicsHeap, uint64_t graphicsHeapOffset, const ElemGraphicsResourceInfo* resourceInfo);
void MetalFreeGraphicsResource(ElemGraphicsResource resource, const ElemFreeGraphicsResourceOptions* options);
ElemGraphicsResourceInfo MetalGetGraphicsResourceInfo(ElemGraphicsResource resource);

void MetalUploadGraphicsBufferData(ElemGraphicsResource resource, uint32_t offset, ElemDataSpan data);
ElemDataSpan MetalDownloadGraphicsBufferData(ElemGraphicsResource resource, const ElemDownloadGraphicsBufferDataOptions* options);
void MetalCopyDataToGraphicsResource(ElemCommandList commandList, const ElemCopyDataToGraphicsResourceParameters* parameters);

ElemGraphicsResourceDescriptor MetalCreateGraphicsResourceDescriptor(ElemGraphicsResource resource, ElemGraphicsResourceDescriptorUsage usage, const ElemGraphicsResourceDescriptorOptions* options);
ElemGraphicsResourceDescriptorInfo MetalGetGraphicsResourceDescriptorInfo(ElemGraphicsResourceDescriptor descriptor);
void MetalFreeGraphicsResourceDescriptor(ElemGraphicsResourceDescriptor descriptor, const ElemFreeGraphicsResourceDescriptorOptions* options);

void MetalProcessGraphicsResourceDeleteQueue(ElemGraphicsDevice graphicsDevice);

ElemGraphicsResource CreateMetalGraphicsResourceFromResource(ElemGraphicsDevice graphicsDevice, ElemGraphicsResourceType type, ElemGraphicsHeap graphicsHeap, ElemGraphicsResourceUsage usage, NS::SharedPtr<MTL::Resource> resource, bool isPresentTexture);

ElemGraphicsSampler MetalCreateGraphicsSampler(ElemGraphicsDevice graphicsDevice, const ElemGraphicsSamplerInfo* samplerInfo);
ElemGraphicsSamplerInfo MetalGetGraphicsSamplerInfo(ElemGraphicsSampler sampler);
void MetalFreeGraphicsSampler(ElemGraphicsSampler sampler, const ElemFreeGraphicsSamplerOptions* options);
