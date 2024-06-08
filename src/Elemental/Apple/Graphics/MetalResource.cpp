#include "MetalResource.h"
#include "MetalGraphicsDevice.h"
#include "SystemDataPool.h"
#include "SystemFunctions.h"
#include "SystemMemory.h"

#define METAL_MAX_GRAPHICSHEAP 32
#define METAL_MAX_TEXTURES UINT16_MAX

SystemDataPool<MetalGraphicsHeapData, MetalGraphicsHeapDataFull> metalGraphicsHeapPool;
SystemDataPool<MetalTextureData, MetalTextureDataFull> metalTexturePool;

MTL::Heap** CurrentMetalHeaps;
uint32_t CurrentMetalHeapsIndex;

void InitMetalResourceMemory()
{
    if (!metalGraphicsHeapPool.Storage)
    {
        metalGraphicsHeapPool = SystemCreateDataPool<MetalGraphicsHeapData, MetalGraphicsHeapDataFull>(MetalGraphicsMemoryArena, METAL_MAX_GRAPHICSHEAP);
        metalTexturePool = SystemCreateDataPool<MetalTextureData, MetalTextureDataFull>(MetalGraphicsMemoryArena, METAL_MAX_TEXTURES);

        CurrentMetalHeaps = SystemPushArray<MTL::Heap*>(MetalGraphicsMemoryArena, METAL_MAX_GRAPHICSHEAP).Pointer;
    }
}

MetalGraphicsHeapData* GetMetalGraphicsHeapData(ElemGraphicsHeap graphicsHeap)
{
    return SystemGetDataPoolItem(metalGraphicsHeapPool, graphicsHeap);
}

MetalGraphicsHeapDataFull* GetMetalGraphicsHeapDataFull(ElemGraphicsHeap graphicsHeap)
{
    return SystemGetDataPoolItemFull(metalGraphicsHeapPool, graphicsHeap);
}

MetalTextureData* GetMetalTextureData(ElemTexture texture)
{
    return SystemGetDataPoolItem(metalTexturePool, texture);
}

MetalTextureDataFull* GetMetalTextureDataFull(ElemTexture texture)
{
    return SystemGetDataPoolItemFull(metalTexturePool, texture);
}

void AddToGlobalMetalHeapList(MTL::Heap* graphicsHeap)
{
    CurrentMetalHeaps[CurrentMetalHeapsIndex++] = graphicsHeap;
}

void RemoveFromGlobalMetalHeapList(MTL::Heap* graphicsHeap)
{
    for (uint32_t i = 0; i < CurrentMetalHeapsIndex; i++)
    {
        if (CurrentMetalHeaps[i] == graphicsHeap)
        {
            if (CurrentMetalHeapsIndex > i + 1)
            {
                for (uint32_t j = i + 1; j < CurrentMetalHeapsIndex; j++)
                {
                    CurrentMetalHeaps[j - 1] = CurrentMetalHeaps[j];
                }
            }

            break;
        }
    }
}

ElemTexture CreateMetalTextureFromResource(ElemGraphicsDevice graphicsDevice, NS::SharedPtr<MTL::Texture> resource, bool isPresentTexture)
{
    InitMetalResourceMemory();

    auto handle = SystemAddDataPoolItem(metalTexturePool, {
        .DeviceObject = resource,
        .Width = (uint32_t)resource->width(),
        .Height = (uint32_t)resource->height(),
        .IsPresentTexture = isPresentTexture,
    }); 

    SystemAddDataPoolItemFull(metalTexturePool, handle, {
        .GraphicsDevice = graphicsDevice
    });

    return handle;
}

MTL::PixelFormat ConvertToMetalTextureFormat(ElemTextureFormat format)
{
    switch (format) 
    {
        case ElemTextureFormat_B8G8R8A8_SRGB:
            return MTL::PixelFormatBGRA8Unorm_sRGB;

        case ElemTextureFormat_B8G8R8A8_UNORM:
            return MTL::PixelFormatBGRA8Unorm;

        case ElemTextureFormat_R16G16B16A16_FLOAT:
            return MTL::PixelFormatRGBA16Float;

        case ElemTextureFormat_R32G32B32A32_FLOAT:
            return MTL::PixelFormatRGBA32Float;
    }
}

MTL::TextureUsage ConvertToMetalTextureUsage(ElemTextureUsage usage)
{
    switch (usage) 
    {
        case ElemTextureUsage_Standard:
            return MTL::TextureUsageShaderRead;

        case ElemTextureUsage_Uav:
            return MTL::TextureUsageShaderRead | MTL::TextureUsageShaderWrite;

        case ElemTextureUsage_RenderTarget:
            return MTL::TextureUsageShaderRead | MTL::TextureUsageRenderTarget;
    }
}

NS::SharedPtr<MTL::TextureDescriptor> CreateMetalTextureDescriptor(const ElemTextureParameters* parameters)
{
    // TODO: Other parameters

    SystemAssert(parameters);

    auto textureDescriptor = NS::TransferPtr(MTL::TextureDescriptor::alloc()->init());
    textureDescriptor->setTextureType(MTL::TextureType2D);
    textureDescriptor->setWidth(parameters->Width);
    textureDescriptor->setHeight(parameters->Height);
    textureDescriptor->setDepth(1);
    textureDescriptor->setMipmapLevelCount(1);
    textureDescriptor->setPixelFormat(ConvertToMetalTextureFormat(parameters->Format));
    textureDescriptor->setSampleCount(1);
    textureDescriptor->setStorageMode(MTL::StorageModePrivate);
    textureDescriptor->setHazardTrackingMode(MTL::HazardTrackingModeUntracked);
    textureDescriptor->setUsage(ConvertToMetalTextureUsage(parameters->Usage));

    return textureDescriptor;
}

ElemGraphicsHeap MetalCreateGraphicsHeap(ElemGraphicsDevice graphicsDevice, uint64_t sizeInBytes, const ElemGraphicsHeapOptions* options)
{
    InitMetalResourceMemory();
    
    SystemAssert(graphicsDevice != ELEM_HANDLE_NULL);

    auto graphicsDeviceData = GetMetalGraphicsDeviceData(graphicsDevice);
    SystemAssert(graphicsDeviceData);

    // TODO: Create other heap types

    auto heapDescriptor = NS::TransferPtr(MTL::HeapDescriptor::alloc()->init());
    heapDescriptor->setStorageMode(MTL::StorageModePrivate);
    heapDescriptor->setType(MTL::HeapTypePlacement);
    heapDescriptor->setSize(sizeInBytes);
    heapDescriptor->setHazardTrackingMode(MTL::HazardTrackingModeUntracked);

    auto graphicsHeap = NS::TransferPtr(graphicsDeviceData->Device->newHeap(heapDescriptor.get()));
    SystemAssertReturnNullHandle(graphicsHeap);
   
    if (MetalDebugLayerEnabled && options && options->DebugName)
    {
        graphicsHeap->setLabel(NS::String::string(options->DebugName, NS::UTF8StringEncoding));
    }
    
    auto handle = SystemAddDataPoolItem(metalGraphicsHeapPool, {
        .DeviceObject = graphicsHeap,
        .SizeInBytes = sizeInBytes,
        .GraphicsDevice = graphicsDevice,
    }); 

    SystemAddDataPoolItemFull(metalGraphicsHeapPool, handle, {
    });

    AddToGlobalMetalHeapList(graphicsHeap.get());

    return handle;
}

void MetalFreeGraphicsHeap(ElemGraphicsHeap graphicsHeap)
{
    SystemAssert(graphicsHeap != ELEM_HANDLE_NULL);

    auto graphicsHeapData = GetMetalGraphicsHeapData(graphicsHeap);
    SystemAssert(graphicsHeapData);

    RemoveFromGlobalMetalHeapList(graphicsHeapData->DeviceObject.get());

    // BUG: For the moment we need to call this method after the free swapchain that 
    // flush the queue. We need to be able to see if all the heap resources have been freed before calling this method.
    graphicsHeapData->DeviceObject.reset();
}

ElemTexture MetalCreateTexture(ElemGraphicsHeap graphicsHeap, uint64_t graphicsHeapOffset, const ElemTextureParameters* parameters)
{
    SystemAssert(graphicsHeap != ELEM_HANDLE_NULL);
    SystemAssert(parameters);

    auto graphicsHeapData = GetMetalGraphicsHeapData(graphicsHeap);
    SystemAssert(graphicsHeapData);

    auto graphicsDeviceData = GetMetalGraphicsDeviceData(graphicsHeapData->GraphicsDevice);
    SystemAssert(graphicsDeviceData);

    auto textureDescriptor = CreateMetalTextureDescriptor(parameters);

    auto texture = NS::TransferPtr(graphicsHeapData->DeviceObject->newTexture(textureDescriptor.get(), graphicsHeapOffset));
    SystemAssertReturnNullHandle(texture);

    if (MetalDebugLayerEnabled && parameters->DebugName)
    {
        texture->setLabel(NS::String::string(parameters->DebugName, NS::UTF8StringEncoding));
    }

    return CreateMetalTextureFromResource(graphicsHeapData->GraphicsDevice, texture, false);
}

void MetalFreeTexture(ElemTexture texture)
{
    // TODO: Defer the real texture delete with a fence!

    auto textureData = GetMetalTextureData(texture);
    SystemAssert(textureData);

    SystemRemoveDataPoolItem(metalTexturePool, texture);
}

ElemShaderDescriptor MetalCreateTextureShaderDescriptor(ElemTexture texture, const ElemTextureShaderDescriptorOptions* options)
{
    return 0;
}

void MetalFreeShaderDescriptor(ElemShaderDescriptor shaderDescriptor)
{
}
