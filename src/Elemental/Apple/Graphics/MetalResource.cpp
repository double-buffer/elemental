#include "MetalResource.h"
#include "MetalGraphicsDevice.h"
#include "MetalCommandList.h"
#include "SystemDataPool.h"
#include "SystemFunctions.h"
#include "SystemMemory.h"

#define METAL_MAX_GRAPHICSHEAP 32

SystemDataPool<MetalGraphicsHeapData, MetalGraphicsHeapDataFull> metalGraphicsHeapPool;
SystemDataPool<MetalResourceData, MetalResourceDataFull> metalResourcePool;
Span<ElemGraphicsResourceDescriptorInfo> metalResourceDescriptorInfos;

void InitMetalResourceMemory()
{
    if (!metalGraphicsHeapPool.Storage)
    {
        metalGraphicsHeapPool = SystemCreateDataPool<MetalGraphicsHeapData, MetalGraphicsHeapDataFull>(MetalGraphicsMemoryArena, METAL_MAX_GRAPHICSHEAP);
        metalResourcePool = SystemCreateDataPool<MetalResourceData, MetalResourceDataFull>(MetalGraphicsMemoryArena, METAL_MAX_RESOURCES);
        metalResourceDescriptorInfos = SystemPushArray<ElemGraphicsResourceDescriptorInfo>(MetalGraphicsMemoryArena, METAL_MAX_RESOURCES);
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

MetalResourceData* GetMetalResourceData(ElemGraphicsResource resource)
{
    return SystemGetDataPoolItem(metalResourcePool, resource);
}

MetalResourceDataFull* GetMetalResourceDataFull(ElemGraphicsResource resource)
{
    return SystemGetDataPoolItemFull(metalResourcePool, resource);
}
        
// TODO: Put that in common code?
ElemGraphicsResourceDescriptorInfo* MetalGetGraphicsResourceDescriptorInfo(ElemGraphicsResourceDescriptor descriptor)
{
    return &metalResourceDescriptorInfos[descriptor];
}

ElemGraphicsResource CreateMetalTextureFromResource(ElemGraphicsDevice graphicsDevice, NS::SharedPtr<MTL::Texture> resource, bool isPresentTexture)
{
    InitMetalResourceMemory();

    auto handle = SystemAddDataPoolItem(metalResourcePool, {
        .DeviceObject = resource,
        .Width = (uint32_t)resource->width(),
        .Height = (uint32_t)resource->height(),
        .IsPresentTexture = isPresentTexture,
    }); 

    SystemAddDataPoolItemFull(metalResourcePool, handle, {
        .GraphicsDevice = graphicsDevice
    });

    return handle;
}

MTL::PixelFormat ConvertToMetalResourceFormat(ElemGraphicsFormat format)
{
    switch (format) 
    {
        case ElemGraphicsFormat_B8G8R8A8_SRGB:
            return MTL::PixelFormatBGRA8Unorm_sRGB;

        case ElemGraphicsFormat_B8G8R8A8_UNORM:
            return MTL::PixelFormatBGRA8Unorm;

        case ElemGraphicsFormat_R16G16B16A16_FLOAT:
            return MTL::PixelFormatRGBA16Float;

        case ElemGraphicsFormat_R32G32B32A32_FLOAT:
            return MTL::PixelFormatRGBA32Float;
    }
}

MTL::TextureUsage ConvertToMetalResourceUsage(ElemGraphicsResourceUsage usage)
{
    switch (usage) 
    {
        case ElemGraphicsResourceUsage_Standard:
            return MTL::TextureUsageShaderRead;

        case ElemGraphicsResourceUsage_Uav:
            return MTL::TextureUsageShaderRead | MTL::TextureUsageShaderWrite;

        case ElemGraphicsResourceUsage_RenderTarget:
            return MTL::TextureUsageShaderRead | MTL::TextureUsageRenderTarget;
    }
}

NS::SharedPtr<MTL::TextureDescriptor> CreateMetalResourceDescriptor(const ElemGraphicsResourceInfo* resourceInfo)
{
    // TODO: Other resourceInfo

    SystemAssert(resourceInfo);

    auto textureDescriptor = NS::TransferPtr(MTL::TextureDescriptor::alloc()->init());
    textureDescriptor->setTextureType(MTL::TextureType2DArray); // TODO: For UAV we need texture2D array?????
    textureDescriptor->setWidth(resourceInfo->Width);
    textureDescriptor->setHeight(resourceInfo->Height);
    textureDescriptor->setDepth(1);
    textureDescriptor->setMipmapLevelCount(1);
    textureDescriptor->setPixelFormat(ConvertToMetalResourceFormat(resourceInfo->Format));
    textureDescriptor->setSampleCount(1);
    textureDescriptor->setStorageMode(MTL::StorageModePrivate);
    textureDescriptor->setHazardTrackingMode(MTL::HazardTrackingModeUntracked);
    textureDescriptor->setUsage(ConvertToMetalResourceUsage(resourceInfo->Usage));

    return textureDescriptor;
}

ElemGraphicsHeap MetalCreateGraphicsHeap(ElemGraphicsDevice graphicsDevice, uint64_t sizeInBytes, const ElemGraphicsHeapOptions* options)
{
    InitMetalResourceMemory();
    
    SystemAssert(graphicsDevice != ELEM_HANDLE_NULL);

    auto graphicsDeviceData = GetMetalGraphicsDeviceData(graphicsDevice);
    SystemAssert(graphicsDeviceData);

    auto graphicsDeviceDataFull = GetMetalGraphicsDeviceDataFull(graphicsDevice);
    SystemAssert(graphicsDeviceDataFull);

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

    graphicsDeviceDataFull->ResidencySet->addAllocation(graphicsHeap.get());
    graphicsDeviceDataFull->ResidencySet->commit();
    graphicsDeviceDataFull->ResidencySet->requestResidency();
    
    auto handle = SystemAddDataPoolItem(metalGraphicsHeapPool, {
        .DeviceObject = graphicsHeap,
        .SizeInBytes = sizeInBytes,
        .GraphicsDevice = graphicsDevice,
    }); 

    SystemAddDataPoolItemFull(metalGraphicsHeapPool, handle, {
    });

    return handle;
}

void MetalFreeGraphicsHeap(ElemGraphicsHeap graphicsHeap)
{
    SystemAssert(graphicsHeap != ELEM_HANDLE_NULL);

    auto graphicsHeapData = GetMetalGraphicsHeapData(graphicsHeap);
    SystemAssert(graphicsHeapData);

    auto graphicsDeviceDataFull = GetMetalGraphicsDeviceDataFull(graphicsHeapData->GraphicsDevice);
    SystemAssert(graphicsDeviceDataFull);

    graphicsDeviceDataFull->ResidencySet->removeAllocation(graphicsHeapData->DeviceObject.get());

    // BUG: For the moment we need to call this method after the free swapchain that 
    // flush the queue. We need to be able to see if all the heap resources have been freed before calling this method.
    graphicsHeapData->DeviceObject.reset();
}

ElemGraphicsResource MetalCreateGraphicsResource(ElemGraphicsHeap graphicsHeap, uint64_t graphicsHeapOffset, const ElemGraphicsResourceInfo* resourceInfo)
{
    SystemAssert(graphicsHeap != ELEM_HANDLE_NULL);
    SystemAssert(resourceInfo);

    auto graphicsHeapData = GetMetalGraphicsHeapData(graphicsHeap);
    SystemAssert(graphicsHeapData);

    auto graphicsDeviceData = GetMetalGraphicsDeviceData(graphicsHeapData->GraphicsDevice);
    SystemAssert(graphicsDeviceData);

    auto textureDescriptor = CreateMetalResourceDescriptor(resourceInfo);

    auto texture = NS::TransferPtr(graphicsHeapData->DeviceObject->newTexture(textureDescriptor.get(), graphicsHeapOffset));
    SystemAssertReturnNullHandle(texture);

    if (MetalDebugLayerEnabled && resourceInfo->DebugName)
    {
        texture->setLabel(NS::String::string(resourceInfo->DebugName, NS::UTF8StringEncoding));
    }

    return CreateMetalTextureFromResource(graphicsHeapData->GraphicsDevice, texture, false);
}

void MetalFreeGraphicsResource(ElemGraphicsResource resource)
{
    // TODO: Defer the real texture delete with a fence!

    auto textureData = GetMetalResourceData(resource);
    SystemAssert(textureData);

    SystemRemoveDataPoolItem(metalResourcePool, resource);
}

ElemGraphicsResourceDescriptor MetalCreateGraphicsResourceDescriptor(const ElemGraphicsResourceDescriptorInfo* descriptorInfo)
{
    SystemAssert(descriptorInfo->Resource != ELEM_HANDLE_NULL);

    auto textureData = GetMetalResourceData(descriptorInfo->Resource);
    SystemAssert(textureData);

    auto textureDataFull = GetMetalResourceDataFull(descriptorInfo->Resource);
    SystemAssert(textureDataFull);

    auto graphicsDeviceData = GetMetalGraphicsDeviceData(textureDataFull->GraphicsDevice);
    SystemAssert(graphicsDeviceData);

    // TODO: We need to use newTextureView if we want to create another view to a specific slice of mip for example

    auto handle = CreateMetalArgumentBufferTextureHandle(graphicsDeviceData->ResourceArgumentBuffer, textureData->DeviceObject.get());
    metalResourceDescriptorInfos[handle] = *descriptorInfo;
    return handle;
}

void MetalUpdateGraphicsResourceDescriptor(ElemGraphicsResourceDescriptor descriptor, const ElemGraphicsResourceDescriptorInfo* descriptorInfo)
{
}

void MetalFreeGraphicsResourceDescriptor(ElemGraphicsResourceDescriptor descriptor)
{
}

void EnsureMetalResourceBarrier(ElemCommandList commandList)
{
    SystemAssert(commandList != ELEM_HANDLE_NULL);

    auto commandListData = GetMetalCommandListData(commandList);
    SystemAssert(commandListData);

    auto commandQueueData = GetMetalCommandQueueData(commandListData->CommandQueue);
    SystemAssert(commandQueueData);

    auto shouldWait = (commandQueueData->ResourceBarrierTypes & MetalResourceBarrierType_Fence) != 0;

    // TODO: Do a resource barrier if we still are inside the same encoder (Fence is false)

    if (commandListData->CommandEncoderType == MetalCommandEncoderType_Render)
    {
        auto renderCommandEncoder = (MTL::RenderCommandEncoder*)commandListData->CommandEncoder.get();
        
        if (shouldWait)
        {
            // TODO: Use the proper stage
            renderCommandEncoder->waitForFence(commandQueueData->ResourceFence.get(), MTL::RenderStageFragment);

            // BUG: It seems that waiting for mesh stage is not working???
            //renderCommandEncoder->waitForFence(commandQueueData->ResourceFence.get(), MTL::RenderStageObject);
            //renderCommandEncoder->waitForFence(commandQueueData->ResourceFence.get(), MTL::RenderStageMesh);
        }
    }
    else if (commandListData->CommandEncoderType == MetalCommandEncoderType_Compute)
    {
        auto computeCommandEncoder = (MTL::ComputeCommandEncoder*)commandListData->CommandEncoder.get();
        //renderCommandEncoder->setBytes(data.Items, data.Length, 2);

        if (shouldWait)
        {
            // TODO: Use the proper stage
            computeCommandEncoder->waitForFence(commandQueueData->ResourceFence.get());
        }
    }

    commandQueueData->ResourceBarrierTypes = 0;
}

void MetalGraphicsResourceBarrier(ElemCommandList commandList, ElemGraphicsResourceDescriptor sourceDescriptor, ElemGraphicsResourceDescriptor destinationDescriptor, const ElemGraphicsResourceBarrierOptions* options)
{
    // TODO: Deffer until needed the waits

    SystemAssert(commandList != ELEM_HANDLE_NULL);

    auto commandListData = GetMetalCommandListData(commandList);
    SystemAssert(commandListData);

    auto commandQueueData = GetMetalCommandQueueData(commandListData->CommandQueue);
    SystemAssert(commandQueueData);

    // TODO: Test code!!!
    commandQueueData->ResourceBarrierTypes |= MetalResourceBarrierType_Texture;
}
