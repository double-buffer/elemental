#include "MetalResource.h"
#include "MetalGraphicsDevice.h"
#include "MetalCommandList.h"
#include "Graphics/ResourceDeleteQueue.h"
#include "Graphics/Resource.h"
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

        // TODO: This should be part of the graphics device data
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
        
MTL::PixelFormat ConvertToMetalResourceFormat(ElemGraphicsFormat format)
{
    switch (format) 
    {
        case ElemGraphicsFormat_B8G8R8A8_SRGB:
            return MTL::PixelFormatBGRA8Unorm_sRGB;

        case ElemGraphicsFormat_B8G8R8A8:
            return MTL::PixelFormatBGRA8Unorm;

        case ElemGraphicsFormat_R16G16B16A16_FLOAT:
            return MTL::PixelFormatRGBA16Float;

        case ElemGraphicsFormat_R32G32B32A32_FLOAT:
            return MTL::PixelFormatRGBA32Float;

        case ElemGraphicsFormat_D32_FLOAT:
            return MTL::PixelFormatDepth32Float;

        case ElemGraphicsFormat_BC7_SRGB:
            return MTL:PixelFormatBC7_RGBAUnorm_sRGB;

        default:
            return MTL::PixelFormatR8Unorm;
    }
}

ElemGraphicsFormat ConvertFromMetalResourceFormat(MTL::PixelFormat format)
{
    switch (format) 
    {
        case MTL::PixelFormatBGRA8Unorm_sRGB:
            return ElemGraphicsFormat_B8G8R8A8_SRGB;

        case MTL::PixelFormatBGRA8Unorm:
            return ElemGraphicsFormat_B8G8R8A8;

        case MTL::PixelFormatRGBA16Float:
            return ElemGraphicsFormat_R16G16B16A16_FLOAT;

        case MTL::PixelFormatRGBA32Float:
            return ElemGraphicsFormat_R32G32B32A32_FLOAT;

        case MTL::PixelFormatDepth32Float:
            return ElemGraphicsFormat_D32_FLOAT;

        case MTL:PixelFormatBC7_RGBAUnorm_sRGB:
            return ElemGraphicsFormat_BC7_SRGB;

        default:
            return ElemGraphicsFormat_Raw;
    }
}

MTL::TextureUsage ConvertToMetalResourceUsage(ElemGraphicsResourceUsage usage)
{
    MTL::TextureUsage result = MTL::TextureUsageShaderRead;

    if (usage & ElemGraphicsResourceUsage_Write)
    {
        result |= MTL::TextureUsageShaderWrite;
    }

    if (usage & ElemGraphicsResourceUsage_RenderTarget ||
        usage & ElemGraphicsResourceUsage_DepthStencil)
    {
        result |= MTL::TextureUsageRenderTarget;
    }

    return result;
}

ElemGraphicsResource CreateMetalGraphicsResourceFromResource(ElemGraphicsDevice graphicsDevice, ElemGraphicsResourceType type, ElemGraphicsResourceUsage usage, NS::SharedPtr<MTL::Resource> resource, bool isPresentTexture)
{
    InitMetalResourceMemory();

    auto width = 0u;
    auto height = 0u;
    auto format = ElemGraphicsFormat_Raw;
    auto mipLevels = 0u;

    if (type == ElemGraphicsResourceType_Texture2D)
    {
        auto texture = (MTL::Texture*)resource.get();
        width = texture->width();
        height = texture->height();
        mipLevels = texture->mipmapLevelCount();
        format = ConvertFromMetalResourceFormat(texture->pixelFormat());
    }
    else
    {
        auto buffer = (MTL::Buffer*)resource.get();
        width = buffer->length();
    }

    auto handle = SystemAddDataPoolItem(metalResourcePool, {
        .DeviceObject = resource,
        .Type = type,
        .Width = width,
        .Height = height,
        .MipLevels = mipLevels,
        .Format = format,
        .Usage = usage,
        .IsPresentTexture = isPresentTexture,
    }); 

    SystemAddDataPoolItemFull(metalResourcePool, handle, {
        .GraphicsDevice = graphicsDevice
    });

    return handle;
}


NS::SharedPtr<MTL::TextureDescriptor> CreateMetalTextureDescriptor(const ElemGraphicsResourceInfo* resourceInfo)
{
    // TODO: Other resourceInfo

    SystemAssert(resourceInfo);

    auto textureDescriptor = NS::TransferPtr(MTL::TextureDescriptor::alloc()->init());
    textureDescriptor->setTextureType(MTL::TextureType2DArray); // TODO: For Write we need texture2D array?????
    textureDescriptor->setWidth(resourceInfo->Width);
    textureDescriptor->setHeight(resourceInfo->Height);
    textureDescriptor->setDepth(1);
    textureDescriptor->setMipmapLevelCount(resourceInfo->MipLevels);
    textureDescriptor->setPixelFormat(ConvertToMetalResourceFormat(resourceInfo->Format));
    textureDescriptor->setSampleCount(1);
    textureDescriptor->setStorageMode(MTL::StorageModeShared);
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

    auto heapDescriptor = NS::TransferPtr(MTL::HeapDescriptor::alloc()->init());
    heapDescriptor->setStorageMode(MTL::StorageModeShared);
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
    
    SystemRemoveDataPoolItem(metalGraphicsHeapPool, graphicsHeap);
}

ElemGraphicsResourceInfo MetalCreateGraphicsBufferResourceInfo(ElemGraphicsDevice graphicsDevice, uint32_t sizeInBytes, ElemGraphicsResourceUsage usage, const ElemGraphicsResourceInfoOptions* options)
{
    SystemAssert(graphicsDevice != ELEM_HANDLE_NULL);

    auto graphicsDeviceData = GetMetalGraphicsDeviceData(graphicsDevice);
    SystemAssert(graphicsDeviceData);

    ElemGraphicsResourceInfo resourceInfo =  
    {
        .Type = ElemGraphicsResourceType_Buffer,
        .Width = sizeInBytes,
        .Usage = usage
    };

    if (options != nullptr)
    {
        resourceInfo.DebugName = options->DebugName;
    }

    auto sizeAndAlignInfo = graphicsDeviceData->Device->heapBufferSizeAndAlign(sizeInBytes, {});
    resourceInfo.Alignment = sizeAndAlignInfo.align;
    resourceInfo.SizeInBytes = sizeAndAlignInfo.size;

    return resourceInfo;

}

ElemGraphicsResourceInfo MetalCreateTexture2DResourceInfo(ElemGraphicsDevice graphicsDevice, uint32_t width, uint32_t height, uint32_t mipLevels, ElemGraphicsFormat format, ElemGraphicsResourceUsage usage, const ElemGraphicsResourceInfoOptions* options)
{    
    SystemAssert(graphicsDevice != ELEM_HANDLE_NULL);

    auto graphicsDeviceData = GetMetalGraphicsDeviceData(graphicsDevice);
    SystemAssert(graphicsDeviceData);

    ElemGraphicsResourceInfo resourceInfo =  
    {
        .Type = ElemGraphicsResourceType_Texture2D,
        .Width = width,
        .Height = height,
        .MipLevels = mipLevels,
        .Format = format,
        .Usage = usage
    };

    if (options != nullptr)
    {
        resourceInfo.DebugName = options->DebugName;
    }

    auto metalTextureDescriptor = CreateMetalTextureDescriptor(&resourceInfo);
    auto sizeAndAlignInfo = graphicsDeviceData->Device->heapTextureSizeAndAlign(metalTextureDescriptor.get());

    resourceInfo.Alignment = sizeAndAlignInfo.align;
    resourceInfo.SizeInBytes = sizeAndAlignInfo.size;

    return resourceInfo;
}

ElemGraphicsResource MetalCreateGraphicsResource(ElemGraphicsHeap graphicsHeap, uint64_t graphicsHeapOffset, const ElemGraphicsResourceInfo* resourceInfo)
{
    SystemAssert(graphicsHeap != ELEM_HANDLE_NULL);
    SystemAssert(resourceInfo);

    auto graphicsHeapData = GetMetalGraphicsHeapData(graphicsHeap);
    SystemAssert(graphicsHeapData);

    auto graphicsDeviceData = GetMetalGraphicsDeviceData(graphicsHeapData->GraphicsDevice);
    SystemAssert(graphicsDeviceData);

    NS::SharedPtr<MTL::Resource> resource;

    if (resourceInfo->Type == ElemGraphicsResourceType_Texture2D)
    {
        if (resourceInfo->Width == 0)
        {
            SystemLogErrorMessage(ElemLogMessageCategory_Graphics, "Texture2D width should not be equals to 0.");
            return ELEM_HANDLE_NULL;
        }

        if (resourceInfo->Height == 0)
        {
            SystemLogErrorMessage(ElemLogMessageCategory_Graphics, "Texture2D height should not be equals to 0.");
            return ELEM_HANDLE_NULL;
        }

        if (resourceInfo->MipLevels == 0)
        {
            SystemLogErrorMessage(ElemLogMessageCategory_Graphics, "Texture2D mipLevels should not be equals to 0.");
            return ELEM_HANDLE_NULL;
        }

        if ((resourceInfo->Usage & ElemGraphicsResourceUsage_RenderTarget) && (resourceInfo->Usage & ElemGraphicsResourceUsage_DepthStencil))
        {
            SystemLogErrorMessage(ElemLogMessageCategory_Graphics, "Texture2D with usage RenderTarget and DepthStencil should not be used together.");
            return ELEM_HANDLE_NULL;
        }

        if (resourceInfo->Usage & ElemGraphicsResourceUsage_DepthStencil && !CheckDepthStencilFormat(resourceInfo->Format))
        {
            SystemLogErrorMessage(ElemLogMessageCategory_Graphics, "Texture2D with usage DepthStencil should use a compatible format.");
            return ELEM_HANDLE_NULL;
        }

        auto textureDescriptor = CreateMetalTextureDescriptor(resourceInfo);
        resource = NS::TransferPtr(graphicsHeapData->DeviceObject->newTexture(textureDescriptor.get(), graphicsHeapOffset));
    }
    else
    {
        if (resourceInfo->Width == 0)
        {
            SystemLogErrorMessage(ElemLogMessageCategory_Graphics, "GraphicsBuffer width should not be equals to 0.");
            return ELEM_HANDLE_NULL;
        }
        
        if (resourceInfo->Usage & ElemGraphicsResourceUsage_RenderTarget)
        {
            SystemLogErrorMessage(ElemLogMessageCategory_Graphics, "GraphicsBuffer usage should not be equals to RenderTarget.");
            return ELEM_HANDLE_NULL;
        }
        
        if (resourceInfo->Usage & ElemGraphicsResourceUsage_DepthStencil)
        {
            SystemLogErrorMessage(ElemLogMessageCategory_Graphics, "GraphicsBuffer usage should not be equals to DepthStencil.");
            return ELEM_HANDLE_NULL;
        }

        resource = NS::TransferPtr(graphicsHeapData->DeviceObject->newBuffer(resourceInfo->Width, MTL::ResourceHazardTrackingModeUntracked, graphicsHeapOffset));
    }
        
    SystemAssertReturnNullHandle(resource);

    if (MetalDebugLayerEnabled && resourceInfo->DebugName)
    {
        resource->setLabel(NS::String::string(resourceInfo->DebugName, NS::UTF8StringEncoding));
    }

    return CreateMetalGraphicsResourceFromResource(graphicsHeapData->GraphicsDevice, resourceInfo->Type, resourceInfo->Usage, resource, false);
}

void MetalFreeGraphicsResource(ElemGraphicsResource resource, const ElemFreeGraphicsResourceOptions* options)
{
    SystemAssert(resource != ELEM_HANDLE_NULL);

    if (options && options->FencesToWait.Length > 0)
    {
        EnqueueResourceDeleteEntry(MetalGraphicsMemoryArena, resource, ResourceDeleteType_Resource, options->FencesToWait);
        return;
    }

    auto resourceData = GetMetalResourceData(resource);

    if (resourceData)
    {
        resourceData->DeviceObject.reset();
        SystemRemoveDataPoolItem(metalResourcePool, resource);
    }
}

ElemGraphicsResourceInfo MetalGetGraphicsResourceInfo(ElemGraphicsResource resource)
{
    SystemAssert(resource != ELEM_HANDLE_NULL);

    auto resourceData = GetMetalResourceData(resource);

    if (!resourceData)
    {
        return {};
    }

    return 
    {
        .Type = resourceData->Type,
        .Width = resourceData->Width,
        .Height = resourceData->Height,
        .MipLevels = resourceData->MipLevels,
        .Format = resourceData->Format,
        .Usage = resourceData->Usage
    };
}
/*
ElemDataSpan MetalGetGraphicsResourceDataSpan(ElemGraphicsResource resource)
{
    SystemAssert(resource != ELEM_HANDLE_NULL);

    auto resourceData = GetMetalResourceData(resource);
    SystemAssert(resourceData);

    if (resourceData->Type != ElemGraphicsResourceType_Buffer)
    {
        SystemLogErrorMessage(ElemLogMessageCategory_Graphics, "GetGraphicsResourceDataSpan only works with graphics buffers.");
        return {};
    }

    auto dataPointer = ((MTL::Buffer*)resourceData->DeviceObject.get())->contents();

    return
    {
        .Items = (uint8_t*)dataPointer,
        .Length = resourceData->Width
    }; 
}*/

void MetalUploadGraphicsBufferData(ElemGraphicsResource resource, uint32_t offset, ElemDataSpan data)
{
}

ElemDataSpan MetalDownloadGraphicsBufferData(ElemGraphicsResource resource, const ElemDownloadGraphicsBufferDataOptions* options)
{
    /*auto offset = 0u;
    auto sizeInBytes = resourceData->Width;

    if (options)
    {
        offset = options->Offset;

        if (options->SizeInBytes != 0)
        {
            sizeInBytes = options->SizeInBytes;
        }
    }

    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto downloadedData = SystemPushArray<uint8_t>(directX12ReadBackMemoryArena, sizeInBytes);
    memcpy(downloadedData.Pointer, (uint8_t*)resourceData->CpuDataPointer + offset, sizeInBytes);

	return { .Items = downloadedData.Pointer, .Length = (uint32_t)downloadedData.Length };*/
    return {};
}

void MetalCopyDataToGraphicsResource(ElemCommandList commandList, const ElemCopyDataToGraphicsResourceParameters* parameters)
{
}

ElemGraphicsResourceDescriptor MetalCreateGraphicsResourceDescriptor(ElemGraphicsResource resource, ElemGraphicsResourceDescriptorUsage usage, const ElemGraphicsResourceDescriptorOptions* options)
{
    SystemAssert(resource != ELEM_HANDLE_NULL);

    auto resourceData = GetMetalResourceData(resource);
    SystemAssert(resourceData);

    auto resourceDataFull = GetMetalResourceDataFull(resource);
    SystemAssert(resourceDataFull);

    auto graphicsDeviceData = GetMetalGraphicsDeviceData(resourceDataFull->GraphicsDevice);
    SystemAssert(graphicsDeviceData);

    // TODO: We need to use newTextureView if we want to create another view to a specific slice of mip for example
    auto handle = -1;

    if (resourceData->Type == ElemGraphicsResourceType_Texture2D)
    {
        if ((usage & ElemGraphicsResourceDescriptorUsage_Write) && !(resourceData->Usage & ElemGraphicsResourceUsage_Write))
        {
            SystemLogErrorMessage(ElemLogMessageCategory_Graphics, "Resource Descriptor write only works with texture created with write usage.");
            return -1;
        }

        handle = CreateMetalArgumentBufferHandleForTexture(graphicsDeviceData->ResourceArgumentBuffer, (MTL::Texture*)resourceData->DeviceObject.get());
    }
    else
    {
        if ((usage & ElemGraphicsResourceDescriptorUsage_Write) && !(resourceData->Usage & ElemGraphicsResourceUsage_Write))
        {
            SystemLogErrorMessage(ElemLogMessageCategory_Graphics, "Resource Descriptor write only works with buffer created with write usage.");
            return -1;
        }
  
        handle = CreateMetalArgumentBufferHandleForBuffer(graphicsDeviceData->ResourceArgumentBuffer, (MTL::Buffer*)resourceData->DeviceObject.get(), resourceData->Width);
    }

    metalResourceDescriptorInfos[handle].Resource = resource;
    metalResourceDescriptorInfos[handle].Usage = usage;
    return handle;
}

ElemGraphicsResourceDescriptorInfo MetalGetGraphicsResourceDescriptorInfo(ElemGraphicsResourceDescriptor descriptor)
{
    if (descriptor == -1)
    {
        SystemLogErrorMessage(ElemLogMessageCategory_Graphics, "Resource Descriptor is invalid.");
        return {};
    }

    return metalResourceDescriptorInfos[descriptor];
}

void MetalFreeGraphicsResourceDescriptor(ElemGraphicsResourceDescriptor descriptor, const ElemFreeGraphicsResourceDescriptorOptions* options)
{
    if (descriptor == -1)
    {
        SystemLogErrorMessage(ElemLogMessageCategory_Graphics, "Resource Descriptor is invalid.");
        return;
    }

    if (options && options->FencesToWait.Length > 0)
    {
        EnqueueResourceDeleteEntry(MetalGraphicsMemoryArena, descriptor, ResourceDeleteType_Descriptor, options->FencesToWait);
        return;
    }
    
    metalResourceDescriptorInfos[descriptor].Resource = ELEM_HANDLE_NULL;
}

void MetalProcessGraphicsResourceDeleteQueue()
{
    ProcessResourceDeleteQueue();
}

