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

void EnsureMetalResourceBarrier(ElemCommandList commandList);

ElemGraphicsResourceDescriptorInfo* MetalGetGraphicsResourceDescriptorInfo(ElemGraphicsResourceDescriptor descriptor);

ElemGraphicsHeap MetalCreateGraphicsHeap(ElemGraphicsDevice graphicsDevice, uint64_t sizeInBytes, const ElemGraphicsHeapOptions* options);
void MetalFreeGraphicsHeap(ElemGraphicsHeap graphicsHeap);

ElemGraphicsResource MetalCreateGraphicsResource(ElemGraphicsHeap graphicsHeap, uint64_t graphicsHeapOffset, const ElemGraphicsResourceInfo* resourceInfo);
void MetalFreeGraphicsResource(ElemGraphicsResource resource);
ElemDataSpan MetalGetGraphicsResourceDataSpan(ElemGraphicsResource resource);

ElemGraphicsResourceDescriptor MetalCreateGraphicsResourceDescriptor(const ElemGraphicsResourceDescriptorInfo* descriptorInfo);
void MetalUpdateGraphicsResourceDescriptor(ElemGraphicsResourceDescriptor descriptor, const ElemGraphicsResourceDescriptorInfo* descriptorInfo);
void MetalFreeGraphicsResourceDescriptor(ElemGraphicsResourceDescriptor descriptor);

void MetalGraphicsResourceBarrier(ElemCommandList commandList, ElemGraphicsResourceDescriptor sourceDescriptor, ElemGraphicsResourceDescriptor destinationDescriptor, const ElemGraphicsResourceBarrierOptions* options);

ElemGraphicsResource CreateMetalGraphicsResourceFromResource(ElemGraphicsDevice graphicsDevice, ElemGraphicsResourceType type, NS::SharedPtr<MTL::Resource> resource, bool isPresentTexture);
