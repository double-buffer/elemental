#include "MetalResource.h"
#include "MetalConfig.h"
#include "MetalGraphicsDevice.h"
#include "MetalResourceBarrier.h"
#include "MetalCommandList.h"
#include "Graphics/UploadBufferPool.h"
#include "Graphics/ResourceDeleteQueue.h"
#include "Graphics/Resource.h"
#include "SystemDataPool.h"
#include "SystemFunctions.h"
#include "SystemMemory.h"

SystemDataPool<MetalGraphicsHeapData, MetalGraphicsHeapDataFull> metalGraphicsHeapPool;
SystemDataPool<MetalResourceData, MetalResourceDataFull> metalResourcePool;
Span<ElemGraphicsResourceDescriptorInfo> metalResourceDescriptorInfos;
Span<MetalGraphicsSamplerInfo> metalSamplerInfos;

MemoryArena metalRaytracingInstanceMemoryArena;
MemoryArena metalReadBackMemoryArena;

Span<ElemGraphicsResource> metalBlasList;
uint32_t metalBlasCount = 0;

thread_local UploadBufferDevicePool<NS::SharedPtr<MTL::Buffer>> threadDirectX12UploadBufferPools[METAL_MAX_DEVICES];

void InitMetalResourceMemory()
{
    if (!metalGraphicsHeapPool.Storage)
    {
        metalGraphicsHeapPool = SystemCreateDataPool<MetalGraphicsHeapData, MetalGraphicsHeapDataFull>(MetalGraphicsMemoryArena, METAL_MAX_GRAPHICSHEAP);
        metalResourcePool = SystemCreateDataPool<MetalResourceData, MetalResourceDataFull>(MetalGraphicsMemoryArena, METAL_MAX_RESOURCES);

        // TODO: This should be part of the graphics device data
        metalResourceDescriptorInfos = SystemPushArray<ElemGraphicsResourceDescriptorInfo>(MetalGraphicsMemoryArena, METAL_MAX_RESOURCES, AllocationState_Reserved);
        metalSamplerInfos = SystemPushArray<MetalGraphicsSamplerInfo>(MetalGraphicsMemoryArena, METAL_MAX_SAMPLERS, AllocationState_Reserved);

        // TODO: Allow to increase the size as a parameter
        metalRaytracingInstanceMemoryArena = SystemAllocateMemoryArena(METAL_RAYTRACING_INSTANCE_MEMORY_ARENA);
        metalReadBackMemoryArena = SystemAllocateMemoryArena(METAL_READBACK_MEMORY_ARENA);

        // HACK: Find a better solution
        metalBlasList = SystemPushArray<ElemGraphicsResource>(MetalGraphicsMemoryArena, 1024);
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

        case ElemGraphicsFormat_BC7:
            return MTL::PixelFormatBC7_RGBAUnorm;

        case ElemGraphicsFormat_BC7_SRGB:
            return MTL::PixelFormatBC7_RGBAUnorm_sRGB;

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

        case MTL::PixelFormatBC7_RGBAUnorm:
            return ElemGraphicsFormat_BC7;

        case MTL::PixelFormatBC7_RGBAUnorm_sRGB:
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

ElemGraphicsResource CreateMetalGraphicsResourceFromResource(ElemGraphicsDevice graphicsDevice, ElemGraphicsResourceType type, ElemGraphicsHeap graphicsHeap, uint64_t heapOffset, uint64_t sizeInBytes, ElemGraphicsResourceUsage usage, NS::SharedPtr<MTL::Resource> resource, bool isPresentTexture)
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
    else if (!(usage & ElemGraphicsResourceUsage_RaytracingAccelerationStructure))
    {
        auto buffer = (MTL::Buffer*)resource.get();
        width = buffer->length();
    }
    else
    {
        width = sizeInBytes;
    }

    auto handle = SystemAddDataPoolItem(metalResourcePool, {
        .DeviceObject = resource,
        .GraphicsHeap = graphicsHeap,
        .HeapOffset = heapOffset,
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

NS::SharedPtr<MTL::TextureDescriptor> CreateMetalTextureDescriptor(const ElemGraphicsResourceInfo* resourceInfo, ElemGraphicsHeapType heapType)
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

    if (heapType == ElemGraphicsHeapType_Gpu)
    {
        textureDescriptor->setStorageMode(MTL::StorageModePrivate);
    }
    else
    {
        textureDescriptor->setStorageMode(MTL::StorageModeShared);
    }

    textureDescriptor->setHazardTrackingMode(MTL::HazardTrackingModeUntracked);
    textureDescriptor->setUsage(ConvertToMetalResourceUsage(resourceInfo->Usage));

    return textureDescriptor;
}

MTL::AttributeFormat ConvertToMetalAttributeFormat(ElemGraphicsFormat format)
{
    if (format == ElemGraphicsFormat_R32G32B32_FLOAT)
    {
        return MTL::AttributeFormatFloat3;
    }
    else if (format == ElemGraphicsFormat_R32G32B32A32_FLOAT)
    {
        return MTL::AttributeFormatFloat4;
    }

    return MTL::AttributeFormatInvalid;
}

MTL::AccelerationStructureUsage ConvertToMetalRaytraceBuildUsage(ElemRaytracingBuildFlags buildOptions)
{
    MTL::AccelerationStructureUsage result = MTL::AccelerationStructureUsageNone;

    if (buildOptions & ElemRaytracingBuildFlags_AllowUpdate)
    {
        result |= MTL::AccelerationStructureUsageRefit;
    }

    if (buildOptions & ElemRaytracingBuildFlags_PreferFastBuild)
    {
        result |= MTL::AccelerationStructureUsagePreferFastBuild;
    }

    return result;
}

NS::SharedPtr<MTL::PrimitiveAccelerationStructureDescriptor> BuildMetalBlasDescriptor(const ElemRaytracingBlasParameters* parameters)
{
    auto vertexBufferResourceData = GetMetalResourceData(parameters->VertexBuffer);
    SystemAssert(vertexBufferResourceData);

    auto indexBufferResourceData = GetMetalResourceData(parameters->IndexBuffer);
    SystemAssert(indexBufferResourceData);

    auto blasDescriptor = NS::TransferPtr(MTL::PrimitiveAccelerationStructureDescriptor::alloc()->init());

    blasDescriptor->setUsage(ConvertToMetalRaytraceBuildUsage(parameters->BuildFlags));

    auto geometryDescriptor = NS::TransferPtr(MTL::AccelerationStructureTriangleGeometryDescriptor::alloc()->init());

    geometryDescriptor->setOpaque(true);
    geometryDescriptor->setIndexType(parameters->IndexFormat == ElemGraphicsFormat_R32_UINT ? MTL::IndexTypeUInt32 : MTL::IndexTypeUInt16);
    geometryDescriptor->setVertexFormat(ConvertToMetalAttributeFormat(parameters->VertexFormat));
    geometryDescriptor->setTriangleCount(parameters->IndexCount / 3);
    geometryDescriptor->setIndexBuffer((MTL::Buffer*)indexBufferResourceData->DeviceObject.get());
    geometryDescriptor->setIndexBufferOffset(parameters->IndexBufferOffset);
    geometryDescriptor->setVertexBuffer((MTL::Buffer*)vertexBufferResourceData->DeviceObject.get());
    geometryDescriptor->setVertexBufferOffset(parameters->VertexBufferOffset);
    geometryDescriptor->setVertexStride(parameters->VertexSizeInBytes);

    MTL::AccelerationStructureTriangleGeometryDescriptor* geometryDescriptors[] = { geometryDescriptor.get() };

    auto geometryDescriptorArray = NS::TransferPtr(NS::Array::alloc()->init((NS::Object**)geometryDescriptors, ARRAYSIZE(geometryDescriptors)));
    blasDescriptor->setGeometryDescriptors(geometryDescriptorArray.get());

    return blasDescriptor;
}

NS::SharedPtr<MTL::InstanceAccelerationStructureDescriptor> BuildMetalTlasDescriptor(MemoryArena memoryArena, const ElemRaytracingTlasParameters* parameters)
{
    auto blasDescriptor = NS::TransferPtr(MTL::InstanceAccelerationStructureDescriptor::alloc()->init());

    blasDescriptor->setUsage(ConvertToMetalRaytraceBuildUsage(parameters->BuildFlags));
    blasDescriptor->setInstanceCount(parameters->InstanceCount); 
    blasDescriptor->setInstanceDescriptorType(MTL::AccelerationStructureInstanceDescriptorTypeUserID);
    blasDescriptor->setInstanceDescriptorStride(68);

    if (parameters->InstanceBuffer)
    {
        auto instanceBufferResourceData = GetMetalResourceData(parameters->InstanceBuffer);
        SystemAssert(instanceBufferResourceData);

        auto blasList = SystemPushArray<MTL::AccelerationStructure*>(memoryArena, metalBlasCount);

        for (uint32_t i = 0; i < metalBlasCount; i++)
        {
            auto blasResourceData = GetMetalResourceData(metalBlasList[i]);
            SystemAssert(blasResourceData);

            auto pointer = (MTL::AccelerationStructure*)blasResourceData->DeviceObject.get();
            blasList[i] = pointer;
        }

        auto blasArray = NS::TransferPtr(NS::Array::alloc()->init((NS::Object**)blasList.Pointer, metalBlasCount));
        blasDescriptor->setInstancedAccelerationStructures(blasArray.get());

        blasDescriptor->setInstanceDescriptorBuffer((MTL::Buffer*)instanceBufferResourceData->DeviceObject.get());
        blasDescriptor->setInstanceDescriptorBufferOffset(parameters->InstanceBufferOffset);
    }
  
    return blasDescriptor;
}

ElemGraphicsHeap MetalCreateGraphicsHeap(ElemGraphicsDevice graphicsDevice, uint64_t sizeInBytes, const ElemGraphicsHeapOptions* options)
{
    InitMetalResourceMemory();

    auto heapType = ElemGraphicsHeapType_Gpu;

    if (options)
    {
        heapType = options->HeapType;
    }
    
    SystemAssert(graphicsDevice != ELEM_HANDLE_NULL);

    auto graphicsDeviceData = GetMetalGraphicsDeviceData(graphicsDevice);
    SystemAssert(graphicsDeviceData);

    auto graphicsDeviceDataFull = GetMetalGraphicsDeviceDataFull(graphicsDevice);
    SystemAssert(graphicsDeviceDataFull);

    auto heapDescriptor = NS::TransferPtr(MTL::HeapDescriptor::alloc()->init());
    heapDescriptor->setType(MTL::HeapTypePlacement);
    heapDescriptor->setSize(sizeInBytes);
    heapDescriptor->setHazardTrackingMode(MTL::HazardTrackingModeUntracked);

    if (heapType == ElemGraphicsHeapType_Gpu)
    {
        heapDescriptor->setStorageMode(MTL::StorageModePrivate);
    }
    else
    {
        heapDescriptor->setStorageMode(MTL::StorageModeShared);
    }

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
        .HeapType = heapType
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

    MTL::SizeAndAlign sizeAndAlignInfo;

    if (usage == ElemGraphicsResourceUsage_RaytracingAccelerationStructure)
    {
        sizeAndAlignInfo = graphicsDeviceData->Device->heapAccelerationStructureSizeAndAlign(sizeInBytes);
    }
    else
    {
        sizeAndAlignInfo = graphicsDeviceData->Device->heapBufferSizeAndAlign(sizeInBytes, {});
    }

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

    auto metalTextureDescriptor = CreateMetalTextureDescriptor(&resourceInfo, ElemGraphicsHeapType_Gpu);
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

        auto textureDescriptor = CreateMetalTextureDescriptor(resourceInfo, graphicsHeapData->HeapType);
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

        if (!(resourceInfo->Usage & ElemGraphicsResourceUsage_RaytracingAccelerationStructure))
        {
            MTL::ResourceOptions metalResourceOptions = MTL::ResourceHazardTrackingModeUntracked;

            if (graphicsHeapData->HeapType == ElemGraphicsHeapType_Gpu)
            {
                metalResourceOptions |= MTL::ResourceStorageModePrivate;
            }
            else 
            {
                metalResourceOptions |= MTL::ResourceStorageModeShared;
            }

            resource = NS::TransferPtr(graphicsHeapData->DeviceObject->newBuffer(resourceInfo->Width, metalResourceOptions, graphicsHeapOffset));
        }
    }
        
    if (!(resourceInfo->Usage & ElemGraphicsResourceUsage_RaytracingAccelerationStructure))
    {
        SystemAssertReturnNullHandle(resource);

        if (MetalDebugLayerEnabled && resourceInfo->DebugName)
        {
            resource->setLabel(NS::String::string(resourceInfo->DebugName, NS::UTF8StringEncoding));
        }
    }

    return CreateMetalGraphicsResourceFromResource(graphicsHeapData->GraphicsDevice, resourceInfo->Type, graphicsHeap, graphicsHeapOffset, resourceInfo->Width, resourceInfo->Usage, resource, false);
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

void MetalUploadGraphicsBufferData(ElemGraphicsResource buffer, uint32_t offset, ElemDataSpan data)
{
    SystemAssert(buffer != ELEM_HANDLE_NULL);

    auto resourceData = GetMetalResourceData(buffer);
    SystemAssert(resourceData);

    auto heapData = GetMetalGraphicsHeapData(resourceData->GraphicsHeap);
    SystemAssert(heapData);

    if (resourceData->Type != ElemGraphicsResourceType_Buffer)
    {
        SystemLogErrorMessage(ElemLogMessageCategory_Graphics, "ElemUploadGraphicsBufferData only works with graphics buffers.");
        return;
    }

    if (heapData->HeapType != ElemGraphicsHeapType_GpuUpload)
    {
        SystemLogErrorMessage(ElemLogMessageCategory_Graphics, "ElemUploadGraphicsBufferData only works with graphics buffers allocated in a GpuUpload heap.");
        return;
    }
    
    auto dataPointer = ((MTL::Buffer*)resourceData->DeviceObject.get())->contents();
    
    auto destinationPointer = (uint8_t*)dataPointer + offset;
    memcpy(destinationPointer, data.Items, data.Length);
}

ElemDataSpan MetalDownloadGraphicsBufferData(ElemGraphicsResource buffer, const ElemDownloadGraphicsBufferDataOptions* options)
{
    SystemAssert(buffer != ELEM_HANDLE_NULL);

    auto resourceData = GetMetalResourceData(buffer);
    SystemAssert(resourceData);

    auto heapData = GetMetalGraphicsHeapData(resourceData->GraphicsHeap);
    SystemAssert(heapData);

    if (resourceData->Type != ElemGraphicsResourceType_Buffer)
    {
        SystemLogErrorMessage(ElemLogMessageCategory_Graphics, "ElemDownloadGraphicsBufferData only works with graphics buffers.");
        return {};
    }

    if (heapData->HeapType != ElemGraphicsHeapType_Readback && heapData->HeapType != ElemGraphicsHeapType_GpuUpload)
    {
        SystemLogErrorMessage(ElemLogMessageCategory_Graphics, "ElemDownloadGraphicsBufferData only works with graphics buffers allocated in a Readback heap or GpuUpload heap. (%d)", heapData->HeapType);
        return {};
    }

    if (heapData->HeapType != ElemGraphicsHeapType_Readback)
    {
        SystemLogWarningMessage(ElemLogMessageCategory_Graphics, "ElemDownloadGraphicsBufferData works faster with graphics buffers allocated in a Readback heap.");
    }

    auto dataPointer = ((MTL::Buffer*)resourceData->DeviceObject.get())->contents();

    auto offset = 0u;
    auto sizeInBytes = resourceData->Width;

    if (options)
    {
        offset = options->Offset;

        if (options->SizeInBytes != 0)
        {
            sizeInBytes = options->SizeInBytes;
        }
    }

    auto downloadedData = SystemPushArray<uint8_t>(metalReadBackMemoryArena, sizeInBytes);
    memcpy(downloadedData.Pointer, (uint8_t*)dataPointer + offset, sizeInBytes);

	return { .Items = downloadedData.Pointer, .Length = (uint32_t)downloadedData.Length };
}

NS::SharedPtr<MTL::Buffer> CreateMetalUploadBuffer(MTL::Device* graphicsDevice, uint64_t sizeInBytes)
{ 
    auto resource = NS::TransferPtr(graphicsDevice->newBuffer(sizeInBytes, MTL::ResourceHazardTrackingModeUntracked));

    if (MetalDebugLayerEnabled)
    {
        resource->setLabel(NS::String::string("ElementalUploadBuffer", NS::UTF8StringEncoding));
    }

    return resource;
}

UploadBufferMemory<NS::SharedPtr<MTL::Buffer>> GetMetalUploadBuffer(ElemGraphicsDevice graphicsDevice, uint64_t alignment, uint64_t sizeInBytes)
{
    auto graphicsDeviceData = GetMetalGraphicsDeviceData(graphicsDevice);
    SystemAssert(graphicsDeviceData);

    auto graphicsIdUnpacked = UnpackSystemDataPoolHandle(graphicsDevice);
    auto uploadBufferPool = &threadDirectX12UploadBufferPools[graphicsIdUnpacked.Index];

    // TODO: Review this
    if (!uploadBufferPool->IsInited)
    {
        SystemLogDebugMessage(ElemLogMessageCategory_Graphics, "Init pool");

        auto poolIndex = SystemAtomicAdd(graphicsDeviceData->CurrentUploadBufferPoolIndex, 1);
        graphicsDeviceData->UploadBufferPools[poolIndex] = uploadBufferPool;
        uploadBufferPool->IsInited = true;
    }

    auto uploadBuffer = GetUploadBufferPoolItem(uploadBufferPool, graphicsDeviceData->UploadBufferGeneration, alignment, sizeInBytes);

    if (uploadBuffer.PoolItem->IsResetNeeded)
    {
        SystemLogDebugMessage(ElemLogMessageCategory_Graphics, "Need to create upload buffer with size: %d...", uploadBuffer.PoolItem->SizeInBytes);

        if (uploadBuffer.PoolItem->Fence.FenceValue > 0)
        {
            SystemLogDebugMessage(ElemLogMessageCategory_Graphics, "Fence: %d", uploadBuffer.PoolItem->Fence.FenceValue);
            MetalWaitForFenceOnCpu(uploadBuffer.PoolItem->Fence);
        }
        
        // TODO: We need another flag to know if we need to delete
        if (uploadBuffer.PoolItem->Buffer.get() != nullptr)
        {
            SystemLogDebugMessage(ElemLogMessageCategory_Graphics, "Need to delete buffer");
            uploadBuffer.PoolItem->Buffer.reset();
        }

        // TODO: Need to update the fence with the command list like in DirectX12

        uploadBuffer.PoolItem->Buffer = CreateMetalUploadBuffer(graphicsDeviceData->Device.get(), uploadBuffer.PoolItem->SizeInBytes);

        uploadBuffer.PoolItem->CpuPointer = (uint8_t*)uploadBuffer.PoolItem->Buffer->contents();
        uploadBuffer.PoolItem->IsResetNeeded = false;
    }

    return uploadBuffer;
}

void MetalCopyDataToGraphicsResource(ElemCommandList commandList, const ElemCopyDataToGraphicsResourceParameters* parameters)
{
    // TODO: Implement file source
    auto stackMemoryArena = SystemGetStackMemoryArena();

    SystemAssert(commandList != ELEM_HANDLE_NULL);

    SystemAssert(parameters);
    SystemAssert(parameters->Resource != ELEM_HANDLE_NULL);

    auto commandListData = GetMetalCommandListData(commandList);
    SystemAssert(commandListData);

    auto graphicsDeviceData = GetMetalGraphicsDeviceData(commandListData->GraphicsDevice);
    SystemAssert(graphicsDeviceData);

    auto resourceData = GetMetalResourceData(parameters->Resource);
    SystemAssert(resourceData);

    ReadOnlySpan<uint8_t> sourceData;

    // TODO: File Source
    if (parameters->SourceType == ElemCopyDataSourceType_Memory)
    {
        sourceData = ReadOnlySpan<uint8_t>(parameters->SourceMemoryData.Items, parameters->SourceMemoryData.Length);
    }
    
    // TODO: Unit test first !!!!!
        
    auto uploadBufferAlignment = 4u;
    auto uploadBufferSizeInBytes = sourceData.Length;

    if (resourceData->Type == ElemGraphicsResourceType_Texture2D)
    {
        if (resourceData->Format == ElemGraphicsFormat_BC7 ||
            resourceData->Format == ElemGraphicsFormat_BC7_SRGB)
        {
            uploadBufferAlignment = 16u;
            uploadBufferSizeInBytes = SystemAlign(uploadBufferSizeInBytes, 16u);
        }
    }

    auto uploadBuffer = GetMetalUploadBuffer(commandListData->GraphicsDevice, uploadBufferAlignment, uploadBufferSizeInBytes);
    SystemAssert(uploadBuffer.Offset + sourceData.Length <= uploadBuffer.PoolItem->SizeInBytes);

    // TODO: Can we do better here?
    auto uploadBufferAlreadyInCommandList = false;

    for (uint32_t i = 0; i < MAX_UPLOAD_BUFFERS; i++)
    {
        if (commandListData->UploadBufferPoolItems[i] == uploadBuffer.PoolItem)
        {
            uploadBufferAlreadyInCommandList = true;
            break;
        }
    }
        
    if (!uploadBufferAlreadyInCommandList)
    {
        commandListData->UploadBufferPoolItems[commandListData->UploadBufferCount++] = uploadBuffer.PoolItem;
    }

    if (commandListData->CommandEncoderType != MetalCommandEncoderType_Copy)
    {
        ResetMetalCommandEncoder(commandList);

        commandListData->CommandEncoder = NS::RetainPtr(commandListData->DeviceObject->blitCommandEncoder()); 
        commandListData->CommandEncoderType = MetalCommandEncoderType_Copy;
    }

    auto copyCommandEncoder = (MTL::BlitCommandEncoder*)commandListData->CommandEncoder.get();
    memcpy(uploadBuffer.PoolItem->CpuPointer + uploadBuffer.Offset, sourceData.Pointer, sourceData.Length);

    if (resourceData->Type == ElemGraphicsResourceType_Buffer)
    {
        copyCommandEncoder->copyFromBuffer(uploadBuffer.PoolItem->Buffer.get(), uploadBuffer.Offset, (MTL::Buffer*)resourceData->DeviceObject.get(), parameters->BufferOffset, sourceData.Length);
    }
    else if (resourceData->Type == ElemGraphicsResourceType_Texture2D)
    {
        auto mipLevel = parameters->TextureMipLevel;

        auto mipWidth  = SystemMax(1u, resourceData->Width  >> mipLevel);
        auto mipHeight = SystemMax(1u, resourceData->Height >> mipLevel);

        auto sourceBytesPerRow = mipWidth * 4;

        if (resourceData->Format == ElemGraphicsFormat_BC7 ||
            resourceData->Format == ElemGraphicsFormat_BC7_SRGB)
        {
            sourceBytesPerRow = ((mipWidth + 3) / 4) * 16;
        }

        copyCommandEncoder->copyFromBuffer(uploadBuffer.PoolItem->Buffer.get(), uploadBuffer.Offset, sourceBytesPerRow, 0, MTL::Size(mipWidth, mipHeight, 1), 
                                           (MTL::Texture*)resourceData->DeviceObject.get(), 0, parameters->TextureMipLevel, MTL::Origin(0, 0, 0));
    }
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
  
        if (resourceData->Usage == ElemGraphicsResourceUsage_RaytracingAccelerationStructure)
        {
            handle = CreateMetalArgumentBufferHandleForAccelerationStructure(graphicsDeviceData->ResourceArgumentBuffer, (MTL::AccelerationStructure*)resourceData->DeviceObject.get(), resourceData->Width);
        }
        else
        {
            handle = CreateMetalArgumentBufferHandleForBuffer(graphicsDeviceData->ResourceArgumentBuffer, (MTL::Buffer*)resourceData->DeviceObject.get(), resourceData->Width);
        }
    }

    if (handle != -1)
    {
        if ((handle % 1000) == 0)
        {
            SystemCommitMemory(MetalGraphicsMemoryArena, &metalResourceDescriptorInfos[handle], 1000 *  sizeof(ElemGraphicsResourceDescriptorInfo));
        }

        metalResourceDescriptorInfos[handle].Resource = resource;
        metalResourceDescriptorInfos[handle].Usage = usage;
    }

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

void MetalProcessGraphicsResourceDeleteQueue(ElemGraphicsDevice graphicsDevice)
{
    auto stackMemoryArena = SystemGetStackMemoryArena();

    ProcessResourceDeleteQueue();
    SystemClearMemoryArena(metalReadBackMemoryArena);
    
    auto graphicsDeviceData = GetMetalGraphicsDeviceData(graphicsDevice);
    SystemAssert(graphicsDeviceData);

    graphicsDeviceData->UploadBufferGeneration++;
    
    for (uint32_t i = 0; i < graphicsDeviceData->UploadBufferPools.Length; i++)
    {
        auto bufferPool = graphicsDeviceData->UploadBufferPools[i];

        if (bufferPool)
        {
            auto uploadBuffersToDelete = GetUploadBufferPoolItemsToDelete(stackMemoryArena, bufferPool, graphicsDeviceData->UploadBufferGeneration);    

            for (uint32_t j = 0; j < uploadBuffersToDelete.Length; j++)
            {
                auto uploadBufferToDelete = uploadBuffersToDelete[j];

                SystemLogDebugMessage(ElemLogMessageCategory_Graphics, "Need to purge upload buffer: Size=%d", uploadBufferToDelete->SizeInBytes);

                uploadBufferToDelete->Buffer.reset();
                uploadBufferToDelete->CurrentOffset = 0;
                uploadBufferToDelete->SizeInBytes = 0;
            }
        }
    }
}

MTL::SamplerMinMagFilter ConvertToMetalMinMagFilter(ElemGraphicsSamplerFilter filter)
{
    switch (filter)
    {
        case ElemGraphicsSamplerFilter_Nearest:
            return MTL::SamplerMinMagFilterNearest;

        case ElemGraphicsSamplerFilter_Linear:
            return MTL::SamplerMinMagFilterLinear;
    }
}

MTL::SamplerMipFilter ConvertToMetalMipFilter(ElemGraphicsSamplerFilter filter)
{
    switch (filter)
    {
        case ElemGraphicsSamplerFilter_Nearest:
            return MTL::SamplerMipFilterNearest;

        case ElemGraphicsSamplerFilter_Linear:
            return MTL::SamplerMipFilterLinear;
    }
}

MTL::SamplerAddressMode ConvertToMetalSamplerAddressMode(ElemGraphicsSamplerAddressMode addressMode)
{
    switch (addressMode)
    {
        case ElemGraphicsSamplerAddressMode_Repeat:
            return MTL::SamplerAddressModeRepeat;

        case ElemGraphicsSamplerAddressMode_RepeatMirror:
            return MTL::SamplerAddressModeMirrorRepeat;

        case ElemGraphicsSamplerAddressMode_ClampToEdge:
            return MTL::SamplerAddressModeClampToEdge;

        case ElemGraphicsSamplerAddressMode_ClampToEdgeMirror:
            return MTL::SamplerAddressModeMirrorClampToEdge;

        case ElemGraphicsSamplerAddressMode_ClampToBorderColor:
            return MTL::SamplerAddressModeClampToBorderColor;
    }
}

ElemGraphicsSampler MetalCreateGraphicsSampler(ElemGraphicsDevice graphicsDevice, const ElemGraphicsSamplerInfo* samplerInfo)
{
    InitMetalResourceMemory();

    auto graphicsDeviceData = GetMetalGraphicsDeviceData(graphicsDevice);
    SystemAssert(graphicsDeviceData);

    auto localSamplerInfo = *samplerInfo;

    if (localSamplerInfo.MaxAnisotropy == 0)
    {
        localSamplerInfo.MaxAnisotropy = 1;
    }

    auto borderColor = MTL::SamplerBorderColorOpaqueBlack;

    if (localSamplerInfo.BorderColor.Red == 1.0f && localSamplerInfo.BorderColor.Green == 1.0f && localSamplerInfo.BorderColor.Blue == 1.0f && localSamplerInfo.BorderColor.Alpha == 1.0f)
    {
        borderColor = MTL::SamplerBorderColorOpaqueWhite;
    }

    auto samplerDescriptor = NS::TransferPtr(MTL::SamplerDescriptor::alloc()->init());
    samplerDescriptor->setSupportArgumentBuffers(true);
    samplerDescriptor->setMinFilter(ConvertToMetalMinMagFilter(localSamplerInfo.MinFilter));
    samplerDescriptor->setMagFilter(ConvertToMetalMinMagFilter(localSamplerInfo.MagFilter));
    samplerDescriptor->setMipFilter(ConvertToMetalMipFilter(localSamplerInfo.MipFilter));
    samplerDescriptor->setSAddressMode(ConvertToMetalSamplerAddressMode(localSamplerInfo.AddressU));
    samplerDescriptor->setTAddressMode(ConvertToMetalSamplerAddressMode(localSamplerInfo.AddressV));
    samplerDescriptor->setRAddressMode(ConvertToMetalSamplerAddressMode(localSamplerInfo.AddressW));
    samplerDescriptor->setMaxAnisotropy(localSamplerInfo.MaxAnisotropy);
    samplerDescriptor->setCompareFunction(ConvertToMetalCompareFunction(localSamplerInfo.CompareFunction));
    samplerDescriptor->setBorderColor(borderColor);
    samplerDescriptor->setLodMinClamp(localSamplerInfo.MinLod);
    samplerDescriptor->setLodMaxClamp(localSamplerInfo.MaxLod == 0 ? 1000 : localSamplerInfo.MaxLod);

    auto sampler = NS::TransferPtr(graphicsDeviceData->Device->newSamplerState(samplerDescriptor.get()));

    auto handle = CreateMetalArgumentBufferHandleForSamplerState(graphicsDeviceData->SamplerArgumentBuffer, sampler.get());

    if ((handle % 1024) == 0)
    {
        SystemCommitMemory(MetalGraphicsMemoryArena, &metalSamplerInfos[handle], 1024 *  sizeof(MetalGraphicsSamplerInfo));
    }

    metalSamplerInfos[handle].MetalSampler = sampler;
    metalSamplerInfos[handle].SamplerInfo = localSamplerInfo;

    return handle;
}

ElemGraphicsSamplerInfo MetalGetGraphicsSamplerInfo(ElemGraphicsSampler sampler)
{
    InitMetalResourceMemory();

    if (sampler == -1)
    {
        SystemLogErrorMessage(ElemLogMessageCategory_Graphics, "Sampler is invalid.");
        return {};
    }

    return metalSamplerInfos[sampler].SamplerInfo;
}

void MetalFreeGraphicsSampler(ElemGraphicsSampler sampler, const ElemFreeGraphicsSamplerOptions* options)
{
    InitMetalResourceMemory();

    if (sampler == -1)
    {
        SystemLogErrorMessage(ElemLogMessageCategory_Graphics, "Sampler is invalid.");
        return;
    }
    
    if (options && options->FencesToWait.Length > 0)
    {
        EnqueueResourceDeleteEntry(MetalGraphicsMemoryArena, sampler, ResourceDeleteType_Sampler, options->FencesToWait);
        return;
    }

    metalSamplerInfos[sampler].MetalSampler.reset();
    metalSamplerInfos[sampler] = {};
}

ElemRaytracingAllocationInfo MetalGetRaytracingBlasAllocationInfo(ElemGraphicsDevice graphicsDevice, const ElemRaytracingBlasParameters* parameters)
{
    auto stackMemoryArena = SystemGetStackMemoryArena();
    // TODO: Add validation
    
    auto graphicsDeviceData = GetMetalGraphicsDeviceData(graphicsDevice);
    SystemAssert(graphicsDeviceData);

    auto descriptor = BuildMetalBlasDescriptor(parameters);
    auto allocationInfo = graphicsDeviceData->Device->accelerationStructureSizes(descriptor.get());

    SystemLogDebugMessage(ElemLogMessageCategory_Graphics, "BLAS Size: %d", allocationInfo.accelerationStructureSize);
    SystemLogDebugMessage(ElemLogMessageCategory_Graphics, "Scratch Size: %d", allocationInfo.buildScratchBufferSize);
    SystemLogDebugMessage(ElemLogMessageCategory_Graphics, "Update Size: %d", allocationInfo.refitScratchBufferSize);

    // TODO: For the moment we are forced to have 16KB align for each accel struct in metal
    // We emulate the storage buffer behavior because metal doesn't allow the creation of an accel struct by specifying 
    // the underlining storage and offset
    auto sizeAndAlignInfo = graphicsDeviceData->Device->heapAccelerationStructureSizeAndAlign(allocationInfo.accelerationStructureSize);
    SystemLogDebugMessage(ElemLogMessageCategory_Graphics, "BLAS Align: %d", sizeAndAlignInfo.align);

    return 
    {
        .Alignment = sizeAndAlignInfo.align,
        .SizeInBytes = allocationInfo.accelerationStructureSize,
        .ScratchSizeInBytes = allocationInfo.buildScratchBufferSize,
        .UpdateScratchSizeInBytes = allocationInfo.refitScratchBufferSize
    };
}

ElemRaytracingAllocationInfo MetalGetRaytracingTlasAllocationInfo(ElemGraphicsDevice graphicsDevice, const ElemRaytracingTlasParameters* parameters)
{
    auto stackMemoryArena = SystemGetStackMemoryArena();
    // TODO: Add validation
    
    auto graphicsDeviceData = GetMetalGraphicsDeviceData(graphicsDevice);
    SystemAssert(graphicsDeviceData);

    auto descriptor = BuildMetalTlasDescriptor(stackMemoryArena, parameters);
    auto allocationInfo = graphicsDeviceData->Device->accelerationStructureSizes(descriptor.get());

    SystemLogDebugMessage(ElemLogMessageCategory_Graphics, "TLAS Size: %d", allocationInfo.accelerationStructureSize);
    SystemLogDebugMessage(ElemLogMessageCategory_Graphics, "Scratch Size: %d", allocationInfo.buildScratchBufferSize);
    SystemLogDebugMessage(ElemLogMessageCategory_Graphics, "Update Size: %d", allocationInfo.refitScratchBufferSize);

    // TODO: For the moment we are forced to have 16KB align for each accel struct in metal
    // We emulate the storage buffer behavior because metal doesn't allow the creation of an accel struct by specifying 
    // the underlining storage and offset
    auto sizeAndAlignInfo = graphicsDeviceData->Device->heapAccelerationStructureSizeAndAlign(allocationInfo.accelerationStructureSize);
    SystemLogDebugMessage(ElemLogMessageCategory_Graphics, "TLAS Align: %d", sizeAndAlignInfo.align);

    return 
    {
        .Alignment = sizeAndAlignInfo.align,
        .SizeInBytes = allocationInfo.accelerationStructureSize,
        .ScratchSizeInBytes = allocationInfo.buildScratchBufferSize,
        .UpdateScratchSizeInBytes = allocationInfo.refitScratchBufferSize
    };
}

ElemGraphicsResourceAllocationInfo MetalGetRaytracingTlasInstanceAllocationInfo(ElemGraphicsDevice graphicsDevice, uint32_t instanceCount)
{
    // TODO: Put 68 bytes in a define

    return 
    {
        .Alignment = 4,
        .SizeInBytes = instanceCount * 68
    };
}

ElemDataSpan MetalEncodeRaytracingTlasInstances(ElemRaytracingTlasInstanceSpan instances)
{
    InitMetalResourceMemory();

    auto result = SystemPushArray<uint8_t>(metalRaytracingInstanceMemoryArena, instances.Length * 68);
    auto currentPointer = result.Pointer;

    for (uint32_t i = 0; i < instances.Length; i++)
    {
        auto instance = &instances.Items[i];

        auto blasResourceData = GetMetalResourceData(instance->BlasResource);
        SystemAssert(blasResourceData);

        auto instanceDescriptor = (MTL::AccelerationStructureUserIDInstanceDescriptor*)currentPointer;

        /*_MTL_OPTIONS(uint32_t, AccelerationStructureInstanceOptions) {
        AccelerationStructureInstanceOptionNone = 0,
        AccelerationStructureInstanceOptionDisableTriangleCulling = 1,
        AccelerationStructureInstanceOptionTriangleFrontFacingWindingCounterClockwise = 2,
        AccelerationStructureInstanceOptionOpaque = 4,
        AccelerationStructureInstanceOptionNonOpaque = 8,*/

        // TODO: Options
        instanceDescriptor->options = MTL::AccelerationStructureInstanceOptionNone;
        instanceDescriptor->mask = instance->InstanceMask;
        instanceDescriptor->accelerationStructureIndex = blasResourceData->BlasIndex;
        instanceDescriptor->userID = instance->InstanceId;
    
        for (uint32_t j = 0; j < 3; j++)
        {
            for (uint32_t k = 0; k < 4; k++)
            {
                instanceDescriptor->transformationMatrix[j][k] = instance->TransformMatrix.Elements[k][j];
            }
        }

        // TODO: Replace magic value
        currentPointer += 68;
    }
    
    return { .Items = (uint8_t*)result.Pointer, .Length = (uint32_t)(instances.Length * 68) };
}

ElemGraphicsResource MetalCreateRaytracingAccelerationStructureResource(ElemGraphicsDevice graphicsDevice, ElemGraphicsResource storageBuffer, const ElemRaytracingAccelerationStructureOptions* options)
{
    InitMetalResourceMemory();

    auto storageBufferResourceData = GetMetalResourceData(storageBuffer);
    SystemAssert(storageBufferResourceData);

    auto graphicsHeapData = GetMetalGraphicsHeapData(storageBufferResourceData->GraphicsHeap);
    SystemAssert(graphicsHeapData);

    SystemLogDebugMessage(ElemLogMessageCategory_Graphics, "Test:%d", storageBufferResourceData->Width);

    auto resource = NS::TransferPtr(graphicsHeapData->DeviceObject->newAccelerationStructure(storageBufferResourceData->Width, storageBufferResourceData->HeapOffset));
    SystemAssertReturnNullHandle(resource);

    if (MetalDebugLayerEnabled && options && options->DebugName)
    {
        resource->setLabel(NS::String::string(options->DebugName, NS::UTF8StringEncoding));
    }

    auto result = CreateMetalGraphicsResourceFromResource(graphicsHeapData->GraphicsDevice, ElemGraphicsResourceType_RaytracingAccelerationStructure, storageBufferResourceData->GraphicsHeap, storageBufferResourceData->Width, storageBufferResourceData->HeapOffset, ElemGraphicsResourceUsage_RaytracingAccelerationStructure, resource, false);

    // HACK: Find a better solution
    auto resourceData = GetMetalResourceData(result);

    resourceData->BlasIndex = metalBlasCount;
    metalBlasList[metalBlasCount++] = result;

    return result;
}

void MetalBuildRaytracingBlas(ElemCommandList commandList, ElemGraphicsResource accelerationStructure, ElemGraphicsResource scratchBuffer, const ElemRaytracingBlasParameters* parameters, const ElemRaytracingBuildOptions* options)
{
    // TODO: Add validation

    auto commandListData = GetMetalCommandListData(commandList);
    SystemAssert(commandListData);

    auto graphicsDeviceData = GetMetalGraphicsDeviceData(commandListData->GraphicsDevice);
    SystemAssert(graphicsDeviceData);

    auto accelerationStructureResourceData = GetMetalResourceData(accelerationStructure);
    SystemAssert(accelerationStructureResourceData);

    auto scratchBufferResourceData = GetMetalResourceData(scratchBuffer);
    SystemAssert(scratchBufferResourceData);
    
    auto blasDescriptor = BuildMetalBlasDescriptor(parameters);

    auto scratchOffset = 0u;

    if (options)
    {
        scratchOffset = options->ScratchOffset;
    }

    InsertMetalResourceBarriersIfNeeded(commandList, ElemGraphicsResourceBarrierSyncType_BuildRaytracingAccelerationStructure);

    if (commandListData->CommandEncoderType != MetalCommandEncoderType_AccelerationStructure)
    {
        ResetMetalCommandEncoder(commandList);

        commandListData->CommandEncoder = NS::RetainPtr(commandListData->DeviceObject->accelerationStructureCommandEncoder()); 
        commandListData->CommandEncoderType = MetalCommandEncoderType_AccelerationStructure;
    }
    
    auto commandEncoder = (MTL::AccelerationStructureCommandEncoder*)commandListData->CommandEncoder.get();

    commandEncoder->buildAccelerationStructure((MTL::AccelerationStructure*)accelerationStructureResourceData->DeviceObject.get(),
                                               blasDescriptor.get(), 
                                               (MTL::Buffer*)scratchBufferResourceData->DeviceObject.get(), scratchOffset);
}

void MetalBuildRaytracingTlas(ElemCommandList commandList, ElemGraphicsResource accelerationStructure, ElemGraphicsResource scratchBuffer, const ElemRaytracingTlasParameters* parameters, const ElemRaytracingBuildOptions* options)
{
    auto stackMemoryArena = SystemGetStackMemoryArena();

    // TODO: Add validation

    auto commandListData = GetMetalCommandListData(commandList);
    SystemAssert(commandListData);

    auto graphicsDeviceData = GetMetalGraphicsDeviceData(commandListData->GraphicsDevice);
    SystemAssert(graphicsDeviceData);

    auto accelerationStructureResourceData = GetMetalResourceData(accelerationStructure);
    SystemAssert(accelerationStructureResourceData);

    auto scratchBufferResourceData = GetMetalResourceData(scratchBuffer);
    SystemAssert(scratchBufferResourceData);
    
    auto tlasDescriptor = BuildMetalTlasDescriptor(stackMemoryArena, parameters);

    auto scratchOffset = 0u;

    if (options)
    {
        scratchOffset = options->ScratchOffset;
    }

    InsertMetalResourceBarriersIfNeeded(commandList, ElemGraphicsResourceBarrierSyncType_BuildRaytracingAccelerationStructure);

    if (commandListData->CommandEncoderType != MetalCommandEncoderType_AccelerationStructure)
    {
        ResetMetalCommandEncoder(commandList);

        commandListData->CommandEncoder = NS::RetainPtr(commandListData->DeviceObject->accelerationStructureCommandEncoder()); 
        commandListData->CommandEncoderType = MetalCommandEncoderType_AccelerationStructure;
    }
    
    auto commandEncoder = (MTL::AccelerationStructureCommandEncoder*)commandListData->CommandEncoder.get();

    commandEncoder->buildAccelerationStructure((MTL::AccelerationStructure*)accelerationStructureResourceData->DeviceObject.get(),
                                               tlasDescriptor.get(), 
                                               (MTL::Buffer*)scratchBufferResourceData->DeviceObject.get(), scratchOffset);
}
