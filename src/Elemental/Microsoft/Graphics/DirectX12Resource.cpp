#include "DirectX12Resource.h"
#include "DirectX12GraphicsDevice.h"
#include "Graphics/ResourceDeleteQueue.h"
#include "SystemDataPool.h"
#include "SystemFunctions.h"
#include "SystemMemory.h"

#define DIRECTX12_MAX_GRAPHICSHEAP 32

SystemDataPool<DirectX12GraphicsHeapData, DirectX12GraphicsHeapDataFull> directX12GraphicsHeapPool;
SystemDataPool<DirectX12GraphicsResourceData, DirectX12GraphicsResourceDataFull> directX12GraphicsResourcePool;

// TODO: This descriptor infos should be linked to the graphics device like the resource desc heaps
Span<ElemGraphicsResourceDescriptorInfo> directX12ResourceDescriptorInfos;

void InitDirectX12ResourceMemory()
{
    if (!directX12GraphicsHeapPool.Storage)
    {
        directX12GraphicsHeapPool = SystemCreateDataPool<DirectX12GraphicsHeapData, DirectX12GraphicsHeapDataFull>(DirectX12MemoryArena, DIRECTX12_MAX_GRAPHICSHEAP);
        directX12GraphicsResourcePool = SystemCreateDataPool<DirectX12GraphicsResourceData, DirectX12GraphicsResourceDataFull>(DirectX12MemoryArena, DIRECTX12_MAX_RESOURCES);
        directX12ResourceDescriptorInfos = SystemPushArray<ElemGraphicsResourceDescriptorInfo>(DirectX12MemoryArena, DIRECTX12_MAX_RESOURCES);
    }
}

DirectX12GraphicsHeapData* GetDirectX12GraphicsHeapData(ElemGraphicsHeap graphicsHeap)
{
    return SystemGetDataPoolItem(directX12GraphicsHeapPool, graphicsHeap);
}

DirectX12GraphicsHeapDataFull* GetDirectX12GraphicsHeapDataFull(ElemGraphicsHeap graphicsHeap)
{
    return SystemGetDataPoolItemFull(directX12GraphicsHeapPool, graphicsHeap);
}

DirectX12GraphicsResourceData* GetDirectX12GraphicsResourceData(ElemGraphicsResource resource)
{
    return SystemGetDataPoolItem(directX12GraphicsResourcePool, resource);
}

DirectX12GraphicsResourceDataFull* GetDirectX12GraphicsResourceDataFull(ElemGraphicsResource resource)
{
    return SystemGetDataPoolItemFull(directX12GraphicsResourcePool, resource);
}

DXGI_FORMAT ConvertDirectX12FormatToSrgbIfNeeded(DXGI_FORMAT format)
{
    switch (format) 
    {
        case DXGI_FORMAT_B8G8R8A8_UNORM:
            return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;

        case DXGI_FORMAT_R8G8B8A8_UNORM:
            return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

        default:
            return format;
    }
}

DXGI_FORMAT ConvertDirectX12FormatWithoutSrgbIfNeeded(DXGI_FORMAT format)
{
    switch (format) 
    {
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
            return DXGI_FORMAT_B8G8R8A8_UNORM;

        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
            return DXGI_FORMAT_R8G8B8A8_UNORM;

        default:
            return format;
    }
}

ElemGraphicsResource CreateDirectX12GraphicsResourceFromResource(ElemGraphicsDevice graphicsDevice, ElemGraphicsResourceType type, ComPtr<ID3D12Resource> resource, bool isPresentTexture)
{
    InitDirectX12ResourceMemory();

    auto graphicsDeviceData = GetDirectX12GraphicsDeviceData(graphicsDevice);
    SystemAssert(graphicsDeviceData);

    auto resourceDesc = resource->GetDesc();
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = {};

    if (resourceDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET)
    {
        rtvHandle = CreateDirectX12DescriptorHandle(graphicsDeviceData->RTVDescriptorHeap);
        
        D3D12_RENDER_TARGET_VIEW_DESC renderTargetViewDesc = 
        {
            .Format = ConvertDirectX12FormatToSrgbIfNeeded(resourceDesc.Format),
            .ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D,
        };
 
        graphicsDeviceData->Device->CreateRenderTargetView(resource.Get(), &renderTargetViewDesc, rtvHandle);
    }
  
    auto handle = SystemAddDataPoolItem(directX12GraphicsResourcePool, {
        .DeviceObject = resource,
        .RtvHandle = rtvHandle,
        .Type = type,
        .DirectX12Format = resourceDesc.Format,
        .DirectX12Flags = resourceDesc.Flags,
        .Width = (uint32_t)resourceDesc.Width,
        .Height = type != ElemGraphicsResourceType_Buffer ? (uint32_t)resourceDesc.Height : 0,
        .MipLevels = resourceDesc.MipLevels,
        .IsPresentTexture = isPresentTexture
    }); 

    SystemAddDataPoolItemFull(directX12GraphicsResourcePool, handle, {
        .GraphicsDevice = graphicsDevice
    });

    return handle;
}

DXGI_FORMAT ConvertToDirectX12TextureFormat(ElemGraphicsFormat format)
{
    switch (format) 
    {
        case ElemGraphicsFormat_B8G8R8A8_SRGB:
            return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;

        case ElemGraphicsFormat_B8G8R8A8_UNORM:
            return DXGI_FORMAT_B8G8R8A8_UNORM;

        case ElemGraphicsFormat_R16G16B16A16_FLOAT:
            return DXGI_FORMAT_R16G16B16A16_FLOAT;

        case ElemGraphicsFormat_R32G32B32A32_FLOAT:
            return DXGI_FORMAT_R32G32B32A32_FLOAT;

        case ElemGraphicsFormat_Raw:
            return DXGI_FORMAT_UNKNOWN;
    }
}

ElemGraphicsFormat ConvertFromDirectX12TextureFormat(DXGI_FORMAT format)
{
    switch (format) 
    {
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
            return ElemGraphicsFormat_B8G8R8A8_SRGB;

        case DXGI_FORMAT_B8G8R8A8_UNORM:
            return ElemGraphicsFormat_B8G8R8A8_UNORM;

        case DXGI_FORMAT_R16G16B16A16_FLOAT:
            return ElemGraphicsFormat_R16G16B16A16_FLOAT;

        case DXGI_FORMAT_R32G32B32A32_FLOAT:
            return ElemGraphicsFormat_R32G32B32A32_FLOAT;

        default:
            return ElemGraphicsFormat_Raw;
    }
}

D3D12_RESOURCE_FLAGS ConvertToDirectX12ResourceFlags(ElemGraphicsResourceUsage usage)
{
    D3D12_RESOURCE_FLAGS result = D3D12_RESOURCE_FLAG_NONE;

    if (usage & ElemGraphicsResourceUsage_Write)
    {
        result |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }
    
    if (usage & ElemGraphicsResourceUsage_RenderTarget)
    {
        result |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    }
    
    return result;
}

ElemGraphicsResourceUsage ConvertFromDirectX12ResourceFlags(D3D12_RESOURCE_FLAGS flags)
{
    ElemGraphicsResourceUsage result = ElemGraphicsResourceUsage_Read;

    if (flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS)
    {
        result = (ElemGraphicsResourceUsage)(result | ElemGraphicsResourceUsage_Write);
    }

    if (flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET)
    {
        result = (ElemGraphicsResourceUsage)(result | ElemGraphicsResourceUsage_RenderTarget);
    }

    return result;
}

D3D12_RESOURCE_DESC1 CreateDirectX12BufferDescription(const ElemGraphicsResourceInfo* resourceInfo)
{
    SystemAssert(resourceInfo);
    
	return 
    {
        .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
        .Alignment = 0,
        .Width = resourceInfo->Width,
        .Height = 1,
        .DepthOrArraySize = 1,
        .MipLevels = 1,
        .Format = ConvertToDirectX12TextureFormat(resourceInfo->Format),
        .SampleDesc =
        {
            .Count = 1,
            .Quality = 0
        },
        .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
        .Flags = ConvertToDirectX12ResourceFlags(resourceInfo->Usage)
    };
}

D3D12_RESOURCE_DESC1 CreateDirectX12TextureDescription(const ElemGraphicsResourceInfo* resourceInfo)
{
    // TODO: other resourceInfo

    SystemAssert(resourceInfo);
    
	return 
    {
        .Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
        .Alignment = 0,
        .Width = resourceInfo->Width,
        .Height = resourceInfo->Height,
        .DepthOrArraySize = 1,
        .MipLevels = (uint16_t)resourceInfo->MipLevels,
        .Format = ConvertToDirectX12TextureFormat(resourceInfo->Format),
        .SampleDesc =
        {
            .Count = 1,
            .Quality = 0
        },
        .Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
        .Flags = ConvertToDirectX12ResourceFlags(resourceInfo->Usage)
    };

    /*
    D3D12_RESOURCE_DIMENSION Dimension;
    UINT64 Alignment;
    UINT64 Width;
    UINT Height;
    UINT16 DepthOrArraySize;
    UINT16 MipLevels;
    DXGI_FORMAT Format;
    DXGI_SAMPLE_DESC SampleDesc;
    D3D12_TEXTURE_LAYOUT Layout;
    D3D12_RESOURCE_FLAGS Flags;
    D3D12_MIP_REGION SamplerFeedbackMipRegion;
    */
    /*
	if (usage == GraphicsTextureUsage::RenderTarget && textureFormat == GraphicsTextureFormat::Depth32Float)
	{
		textureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	}

	else if (usage == GraphicsTextureUsage::RenderTarget) 
	{
		textureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	}

	else if (usage == GraphicsTextureUsage::ShaderWrite) 
	{
		textureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	}*/
}

ElemGraphicsHeap DirectX12CreateGraphicsHeap(ElemGraphicsDevice graphicsDevice, uint64_t sizeInBytes, const ElemGraphicsHeapOptions* options)
{
    auto stackMemoryArena = SystemGetStackMemoryArena();
    InitDirectX12ResourceMemory();
    
    SystemAssert(graphicsDevice != ELEM_HANDLE_NULL);

    auto graphicsDeviceData = GetDirectX12GraphicsDeviceData(graphicsDevice);
    SystemAssert(graphicsDeviceData);

    D3D12_HEAP_PROPERTIES heapProperties =
    {
        .Type = D3D12_HEAP_TYPE_GPU_UPLOAD,
		.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
    };

    if (options && options->HeapType == ElemGraphicsHeapType_Readback)
    {
        heapProperties = graphicsDeviceData->Device->GetCustomHeapProperties(0, D3D12_HEAP_TYPE_READBACK);
    }

    D3D12_HEAP_DESC heapDesc = 
    {
		.SizeInBytes = sizeInBytes,
        .Properties = heapProperties,
		.Flags = D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES
    };

	ComPtr<ID3D12Heap> graphicsHeap;
	AssertIfFailed(graphicsDeviceData->Device->CreateHeap(&heapDesc, IID_PPV_ARGS(graphicsHeap.GetAddressOf())));

    if (DirectX12DebugLayerEnabled && options && options->DebugName)
    {
        graphicsHeap->SetName(SystemConvertUtf8ToWideChar(stackMemoryArena, options->DebugName).Pointer);
    }

    auto handle = SystemAddDataPoolItem(directX12GraphicsHeapPool, {
        .DeviceObject = graphicsHeap,
        .SizeInBytes = sizeInBytes,
        .GraphicsDevice = graphicsDevice,
    }); 

    SystemAddDataPoolItemFull(directX12GraphicsHeapPool, handle, {
        .HeapDescription = heapDesc 
    });

    return handle;
}

void DirectX12FreeGraphicsHeap(ElemGraphicsHeap graphicsHeap)
{
    SystemAssert(graphicsHeap != ELEM_HANDLE_NULL);

    auto graphicsHeapData = GetDirectX12GraphicsHeapData(graphicsHeap);
    SystemAssert(graphicsHeapData);

    // BUG: For the moment we need to call this method after the free swapchain that 
    // flush the queue. We need to be able to see if all the heap resources have been freed before calling this method.
    graphicsHeapData->DeviceObject.Reset();
        
    SystemRemoveDataPoolItem(directX12GraphicsHeapPool, graphicsHeap);
}

ElemGraphicsResourceInfo DirectX12CreateGraphicsBufferResourceInfo(ElemGraphicsDevice graphicsDevice, uint32_t sizeInBytes, ElemGraphicsResourceUsage usage, const ElemGraphicsResourceInfoOptions* options)
{
    SystemAssert(graphicsDevice != ELEM_HANDLE_NULL);

    auto graphicsDeviceData = GetDirectX12GraphicsDeviceData(graphicsDevice);
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

    auto resourceDescription = CreateDirectX12BufferDescription(&resourceInfo);
    auto sizeAndAlignInfo = graphicsDeviceData->Device->GetResourceAllocationInfo2(0, 1, &resourceDescription, nullptr);
    resourceInfo.Alignment = sizeAndAlignInfo.Alignment;
    resourceInfo.SizeInBytes = sizeAndAlignInfo.SizeInBytes;

    return resourceInfo;
}

ElemGraphicsResourceInfo DirectX12CreateTexture2DResourceInfo(ElemGraphicsDevice graphicsDevice, uint32_t width, uint32_t height, uint32_t mipLevels, ElemGraphicsFormat format, ElemGraphicsResourceUsage usage, const ElemGraphicsResourceInfoOptions* options)
{
    SystemAssert(graphicsDevice != ELEM_HANDLE_NULL);

    auto graphicsDeviceData = GetDirectX12GraphicsDeviceData(graphicsDevice);
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

    auto resourceDescription = CreateDirectX12TextureDescription(&resourceInfo);
    auto sizeAndAlignInfo = graphicsDeviceData->Device->GetResourceAllocationInfo2(0, 1, &resourceDescription, nullptr);
    resourceInfo.Alignment = sizeAndAlignInfo.Alignment;
    resourceInfo.SizeInBytes = sizeAndAlignInfo.SizeInBytes;

    return resourceInfo;
}

ElemGraphicsResource DirectX12CreateGraphicsResource(ElemGraphicsHeap graphicsHeap, uint64_t graphicsHeapOffset, const ElemGraphicsResourceInfo* resourceInfo)
{
    auto stackMemoryArena = SystemGetStackMemoryArena();
    SystemAssert(graphicsHeap != ELEM_HANDLE_NULL);
    SystemAssert(resourceInfo);

    auto graphicsHeapData = GetDirectX12GraphicsHeapData(graphicsHeap);
    SystemAssert(graphicsHeapData);

    auto graphicsDeviceData = GetDirectX12GraphicsDeviceData(graphicsHeapData->GraphicsDevice);
    SystemAssert(graphicsDeviceData);
    
    D3D12_BARRIER_LAYOUT initialState = D3D12_BARRIER_LAYOUT_COMMON;
    D3D12_CLEAR_VALUE* clearValue = nullptr;

    D3D12_RESOURCE_DESC1 resourceDescription = {}; 

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

        resourceDescription = CreateDirectX12TextureDescription(resourceInfo);
    }
    else
    {
        if (resourceInfo->Width == 0)
        {
            SystemLogErrorMessage(ElemLogMessageCategory_Graphics, "GraphicsBuffer width should not be equals to 0.");
            return ELEM_HANDLE_NULL;
        }
        
        if ((resourceInfo->Usage & ElemGraphicsResourceUsage_RenderTarget))
        {
            SystemLogErrorMessage(ElemLogMessageCategory_Graphics, "GraphicsBuffer usage should not be equals to RenderTarget.");
            return ELEM_HANDLE_NULL;
        }

        initialState = D3D12_BARRIER_LAYOUT_UNDEFINED;
        resourceDescription = CreateDirectX12BufferDescription(resourceInfo);
    }

    ComPtr<ID3D12Resource> resource;
	AssertIfFailed(graphicsDeviceData->Device->CreatePlacedResource2(graphicsHeapData->DeviceObject.Get(), 
                                                                     graphicsHeapOffset, 
                                                                     &resourceDescription, 
                                                                     initialState, 
                                                                     clearValue, 
                                                                     0,
                                                                     nullptr,
                                                                     IID_PPV_ARGS(resource.GetAddressOf())));

    if (DirectX12DebugLayerEnabled && resourceInfo->DebugName)
    {
        resource->SetName(SystemConvertUtf8ToWideChar(stackMemoryArena, resourceInfo->DebugName).Pointer);
    }

    return CreateDirectX12GraphicsResourceFromResource(graphicsHeapData->GraphicsDevice, resourceInfo->Type, resource, false);
}

void DirectX12FreeGraphicsResource(ElemGraphicsResource resource, const ElemFreeGraphicsResourceOptions* options)
{
    SystemAssert(resource != ELEM_HANDLE_NULL);

    if (options && options->FencesToWait.Length > 0)
    {
        EnqueueResourceDeleteEntry(DirectX12MemoryArena, resource, ResourceDeleteType_Resource, options->FencesToWait);
        return;
    }

    auto resourceData = GetDirectX12GraphicsResourceData(resource);

    if (resourceData)
    {
        auto resourceDataFull = GetDirectX12GraphicsResourceDataFull(resource);
        SystemAssert(resourceDataFull);

        auto graphicsDeviceData = GetDirectX12GraphicsDeviceData(resourceDataFull->GraphicsDevice);
        SystemAssert(graphicsDeviceData);

        if (resourceData->DeviceObject)
        {
            resourceData->DeviceObject.Reset();
        }

        SystemRemoveDataPoolItem(directX12GraphicsResourcePool, resource);
    }
}

ElemGraphicsResourceInfo DirectX12GetGraphicsResourceInfo(ElemGraphicsResource resource)
{
    SystemAssert(resource != ELEM_HANDLE_NULL);

    auto resourceData = GetDirectX12GraphicsResourceData(resource);

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
        .Format = ConvertFromDirectX12TextureFormat(resourceData->DirectX12Format),
        .Alignment = 0,
        .SizeInBytes = 0,
        .Usage = ConvertFromDirectX12ResourceFlags(resourceData->DirectX12Flags)
    };
}

ElemDataSpan DirectX12GetGraphicsResourceDataSpan(ElemGraphicsResource resource)
{
    SystemAssert(resource != ELEM_HANDLE_NULL);

    auto resourceData = GetDirectX12GraphicsResourceData(resource);
    SystemAssert(resourceData);

    if (resourceData->Type != ElemGraphicsResourceType_Buffer)
    {
        SystemLogErrorMessage(ElemLogMessageCategory_Graphics, "GetGraphicsResourceDataSpan only works with graphics buffers.");
        return {};
    }

	if (resourceData->CpuDataPointer == nullptr)
	{
	    D3D12_RANGE readRange = { 0, 0 };
	    resourceData->DeviceObject->Map(0, &readRange, &resourceData->CpuDataPointer);
	}

	return { .Items = (uint8_t*)resourceData->CpuDataPointer, .Length = resourceData->Width };
}

ElemGraphicsResourceDescriptor DirectX12CreateGraphicsResourceDescriptor(ElemGraphicsResource resource, ElemGraphicsResourceDescriptorUsage usage, const ElemGraphicsResourceDescriptorOptions* options)
{
    SystemAssert(resource != ELEM_HANDLE_NULL);

    auto resourceData = GetDirectX12GraphicsResourceData(resource);
    SystemAssert(resourceData);

    auto resourceUsage = ConvertFromDirectX12ResourceFlags(resourceData->DirectX12Flags);

    if (resourceData->Type == ElemGraphicsResourceType_Texture2D)
    {
        if (usage == ElemGraphicsResourceDescriptorUsage_Write && resourceUsage != ElemGraphicsResourceUsage_Write)
        {
            SystemLogErrorMessage(ElemLogMessageCategory_Graphics, "Resource Descriptor write only works with texture created with write usage.");
            return -1;
        }
    }
    else
    {
        if (usage == ElemGraphicsResourceDescriptorUsage_Write && resourceUsage != ElemGraphicsResourceUsage_Write)
        {
            SystemLogErrorMessage(ElemLogMessageCategory_Graphics, "Resource Descriptor write only works with buffer created with write usage.");
            return -1;
        }
    }

    auto resourceDataFull = GetDirectX12GraphicsResourceDataFull(resource);
    SystemAssert(resourceDataFull);

    auto graphicsDeviceData = GetDirectX12GraphicsDeviceData(resourceDataFull->GraphicsDevice);
    SystemAssert(graphicsDeviceData);

    auto textureMipIndex = 0u;

    if (options)
    {
        textureMipIndex = options->TextureMipIndex;
    }

    auto descriptorHeap = graphicsDeviceData->ResourceDescriptorHeap;
    auto descriptorHandle = CreateDirectX12DescriptorHandle(descriptorHeap);

    if (usage == ElemGraphicsResourceDescriptorUsage_Read)
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = 
        {
            .Format = resourceData->DirectX12Format,
            .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
        };

        if (resourceData->Type == ElemGraphicsResourceType_Texture2D)
        {
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D =
            {
                .MostDetailedMip = textureMipIndex,
                .MipLevels = resourceData->MipLevels - textureMipIndex
            };
        }
        else
        {
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
            srvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
            srvDesc.Buffer =
            {
                .NumElements = resourceData->Width / 4,
                .Flags = D3D12_BUFFER_SRV_FLAG_RAW
            };
        }

        graphicsDeviceData->Device->CreateShaderResourceView(resourceData->DeviceObject.Get(), &srvDesc, descriptorHandle);
    }
    else if (usage == ElemGraphicsResourceDescriptorUsage_Write)
    {
        D3D12_UNORDERED_ACCESS_VIEW_DESC uavViewDesc = 
        {
            .Format = ConvertDirectX12FormatWithoutSrgbIfNeeded(resourceData->DirectX12Format),
        };

        if (resourceData->Type == ElemGraphicsResourceType_Texture2D)
        {
            uavViewDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
            uavViewDesc.Texture2D = 
            {
                .MipSlice = textureMipIndex
            };
        }
        else
        {
            uavViewDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
            uavViewDesc.Format = DXGI_FORMAT_R32_TYPELESS;
            uavViewDesc.Buffer =
            {
                .NumElements = resourceData->Width / 4,
                .Flags = D3D12_BUFFER_UAV_FLAG_RAW
            };
        }
        
        graphicsDeviceData->Device->CreateUnorderedAccessView(resourceData->DeviceObject.Get(), nullptr, &uavViewDesc, descriptorHandle);
    }

    // TODO: Error message if combinasions
    
    auto index = ConvertDirectX12DescriptorHandleToIndex(descriptorHeap, descriptorHandle);

    // BUG: Because we have 2 separate descriptor heaps the index is not correct here
    // We need to find another way to map that
    directX12ResourceDescriptorInfos[index].Resource = resource;
    directX12ResourceDescriptorInfos[index].Usage = usage;

    return index;
}

ElemGraphicsResourceDescriptorInfo DirectX12GetGraphicsResourceDescriptorInfo(ElemGraphicsResourceDescriptor descriptor)
{
    if (descriptor == -1)
    {
        SystemLogErrorMessage(ElemLogMessageCategory_Graphics, "Resource Descriptor is invalid.");
        return {};
    }

    return directX12ResourceDescriptorInfos[descriptor];
}

void DirectX12FreeGraphicsResourceDescriptor(ElemGraphicsResourceDescriptor descriptor, const ElemFreeGraphicsResourceDescriptorOptions* options)
{
    if (descriptor == -1)
    {
        SystemLogErrorMessage(ElemLogMessageCategory_Graphics, "Resource Descriptor is invalid.");
        return;
    }

    if (options && options->FencesToWait.Length > 0)
    {
        EnqueueResourceDeleteEntry(DirectX12MemoryArena, descriptor, ResourceDeleteType_Descriptor, options->FencesToWait);
        return;
    }

    directX12ResourceDescriptorInfos[descriptor].Resource = ELEM_HANDLE_NULL;
}

void DirectX12ProcessGraphicsResourceDeleteQueue(void)
{
    ProcessResourceDeleteQueue();
}
