#include "DirectX12Resource.h"
#include "DirectX12GraphicsDevice.h"
#include "SystemDataPool.h"
#include "SystemFunctions.h"
#include "SystemMemory.h"

#define DIRECTX12_MAX_GRAPHICSHEAP 32

SystemDataPool<DirectX12GraphicsHeapData, DirectX12GraphicsHeapDataFull> directX12GraphicsHeapPool;
SystemDataPool<DirectX12GraphicsResourceData, DirectX12GraphicsResourceDataFull> directX12GraphicsResourcePool;
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

DirectX12GraphicsResourceData* GetDirectX12GraphicsResourceData(ElemGraphicsResource texture)
{
    return SystemGetDataPoolItem(directX12GraphicsResourcePool, texture);
}

DirectX12GraphicsResourceDataFull* GetDirectX12GraphicsResourceDataFull(ElemGraphicsResource texture)
{
    return SystemGetDataPoolItemFull(directX12GraphicsResourcePool, texture);
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

ElemGraphicsResource CreateDirectX12TextureFromResource(ElemGraphicsDevice graphicsDevice, ComPtr<ID3D12Resource> resource, bool isPresentTexture)
{
    InitDirectX12ResourceMemory();

    auto graphicsDeviceData = GetDirectX12GraphicsDeviceData(graphicsDevice);
    SystemAssert(graphicsDeviceData);

    auto resourceDesc = resource->GetDesc();
  
    auto handle = SystemAddDataPoolItem(directX12GraphicsResourcePool, {
        .DeviceObject = resource,
        .DirectX12Format = resourceDesc.Format,
        .Width = (uint32_t)resourceDesc.Width,
        .Height = (uint32_t)resourceDesc.Height,
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
    }
}

D3D12_RESOURCE_FLAGS ConvertToDirectX12ResourceFlags(ElemGraphicsResourceUsage usage)
{
    switch (usage) 
    {
        case ElemGraphicsResourceUsage_Standard:
            return D3D12_RESOURCE_FLAG_NONE;

        case ElemGraphicsResourceUsage_Uav:
            return D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

        case ElemGraphicsResourceUsage_RenderTarget:
            return D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    }
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
        .MipLevels = 1,
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

    // TODO: Create other heap types

    D3D12_HEAP_DESC heapDesc = 
    {
		.SizeInBytes = sizeInBytes,
        .Properties = 
        {
            .Type = D3D12_HEAP_TYPE_GPU_UPLOAD,
		    .CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
        },
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
}

ElemGraphicsResourceInfo DirectX12CreateGraphicsBufferResourceInfo(ElemGraphicsDevice graphicsDevice, uint32_t sizeInBytes, const ElemGraphicsResourceInfoOptions* options)
{
    return {};
}

ElemGraphicsResourceInfo DirectX12CreateTexture2DResourceInfo(ElemGraphicsDevice graphicsDevice, uint32_t width, uint32_t height, uint32_t mipLevels, ElemGraphicsFormat format, const ElemGraphicsResourceInfoOptions* options)
{
    return {};
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

    auto resourceDescription = CreateDirectX12TextureDescription(resourceInfo);

    // TODO: 
    D3D12_BARRIER_LAYOUT initialState = D3D12_BARRIER_LAYOUT_COMMON;
    D3D12_CLEAR_VALUE* clearValue = nullptr;

    ComPtr<ID3D12Resource> texture;
	AssertIfFailed(graphicsDeviceData->Device->CreatePlacedResource2(graphicsHeapData->DeviceObject.Get(), 
                                                                     graphicsHeapOffset, 
                                                                     &resourceDescription, 
                                                                     initialState, 
                                                                     clearValue, 
                                                                     0,
                                                                     nullptr,
                                                                     IID_PPV_ARGS(texture.GetAddressOf())));

    if (DirectX12DebugLayerEnabled && resourceInfo->DebugName)
    {
        texture->SetName(SystemConvertUtf8ToWideChar(stackMemoryArena, resourceInfo->DebugName).Pointer);
    }

    return CreateDirectX12TextureFromResource(graphicsHeapData->GraphicsDevice, texture, false);
}

void DirectX12FreeGraphicsResource(ElemGraphicsResource resource, const ElemFreeGraphicsResourceOptions* options)
{
    SystemAssert(resource != ELEM_HANDLE_NULL);

    auto resourceData = GetDirectX12GraphicsResourceData(resource);
    SystemAssert(resourceData);

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

ElemGraphicsResourceInfo DirectX12GetGraphicsResourceInfo(ElemGraphicsResource resource)
{
    return {};
}

ElemDataSpan DirectX12GetGraphicsResourceDataSpan(ElemGraphicsResource resource)
{
}

ElemGraphicsResourceDescriptor DirectX12CreateGraphicsResourceDescriptor(ElemGraphicsResource resource, ElemGraphicsResourceUsage usage, const ElemGraphicsResourceDescriptorOptions* options)
{
    SystemAssert(resource != ELEM_HANDLE_NULL);

    auto resourceData = GetDirectX12GraphicsResourceData(resource);
    SystemAssert(resourceData);

    auto resourceDataFull = GetDirectX12GraphicsResourceDataFull(resource);
    SystemAssert(resourceDataFull);

    auto graphicsDeviceData = GetDirectX12GraphicsDeviceData(resourceDataFull->GraphicsDevice);
    SystemAssert(graphicsDeviceData);

    DirectX12DescriptorHeap descriptorHeap;

    if (usage == ElemGraphicsResourceUsage_RenderTarget)
    {
        descriptorHeap = graphicsDeviceData->RTVDescriptorHeap;
    }
    else 
    {
        descriptorHeap = graphicsDeviceData->ResourceDescriptorHeap;
    }
        
    auto descriptorHandle = CreateDirectX12DescriptorHandle(graphicsDeviceData->ResourceDescriptorHeap);

    switch (usage)
    {
        case ElemGraphicsResourceUsage_Standard:
        {
            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = 
            {
                .Format = resourceData->DirectX12Format,
                .ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
                .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
                .Texture2D =
                {
                    .MostDetailedMip = options->TextureMipIndex,
                    .MipLevels = resourceData->MipLevels - options->TextureMipIndex
                }
            };

		    graphicsDeviceData->Device->CreateShaderResourceView(resourceData->DeviceObject.Get(), &srvDesc, descriptorHandle);
            break;
        }
        case ElemGraphicsResourceUsage_Uav:
        {
            D3D12_UNORDERED_ACCESS_VIEW_DESC uavViewDesc = 
            {
                .Format = resourceData->DirectX12Format,
                .ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D,
                .Texture2D = 
                {
                    .MipSlice = options->TextureMipIndex
                }
            };
            
            graphicsDeviceData->Device->CreateUnorderedAccessView(resourceData->DeviceObject.Get(), nullptr, &uavViewDesc, descriptorHandle);
            break;
        }
        case ElemGraphicsResourceUsage_RenderTarget:
        {
            D3D12_RENDER_TARGET_VIEW_DESC renderTargetViewDesc = 
            {
                .Format = ConvertDirectX12FormatToSrgbIfNeeded(resourceData->DirectX12Format),
                .ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D,
            };
     
            graphicsDeviceData->Device->CreateRenderTargetView(resourceData->DeviceObject.Get(), &renderTargetViewDesc, descriptorHandle);
            break;
        }
    }

    return ConvertDirectX12DescriptorHandleToIndex(descriptorHeap, descriptorHandle);
}

ElemGraphicsResourceDescriptorInfo DirectX12GetGraphicsResourceDescriptorInfo(ElemGraphicsResourceDescriptor descriptor)
{
}

void DirectX12FreeGraphicsResourceDescriptor(ElemGraphicsResourceDescriptor descriptor, const ElemFreeGraphicsResourceDescriptorOptions* options)
{
}

void DirectX12ProcessGraphicsResourceDeleteQueue(void)
{
}

void DirectX12GraphicsResourceBarrier(ElemCommandList commandList, ElemGraphicsResourceDescriptor sourceDescriptor, ElemGraphicsResourceDescriptor destinationDescriptor, const ElemGraphicsResourceBarrierOptions* options)
{
}
