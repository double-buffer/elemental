#pragma once

#include "Elemental.h"

struct MetalGraphicsHeapData
{
    NS::SharedPtr<MTL::Heap> DeviceObject;
    uint64_t SizeInBytes;
    ElemGraphicsDevice GraphicsDevice;
};

struct MetalGraphicsHeapDataFull
{
    uint32_t Reserved;
};

struct MetalResourceData
{
    NS::SharedPtr<MTL::Resource> DeviceObject;
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

ElemGraphicsResourceDescriptor MetalCreateGraphicsResourceDescriptor(ElemGraphicsResource resource, ElemGraphicsResourceDescriptorUsage usage, const ElemGraphicsResourceDescriptorOptions* options);
ElemGraphicsResourceDescriptorInfo MetalGetGraphicsResourceDescriptorInfo(ElemGraphicsResourceDescriptor descriptor);
void MetalFreeGraphicsResourceDescriptor(ElemGraphicsResourceDescriptor descriptor, const ElemFreeGraphicsResourceDescriptorOptions* options);

void MetalProcessGraphicsResourceDeleteQueue();

ElemGraphicsResource CreateMetalGraphicsResourceFromResource(ElemGraphicsDevice graphicsDevice, ElemGraphicsResourceType type, ElemGraphicsResourceUsage usage, NS::SharedPtr<MTL::Resource> resource, bool isPresentTexture);
