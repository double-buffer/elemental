#include "DirectX12Resource.h"
#include "DirectX12GraphicsDevice.h"
#include "SystemDataPool.h"
#include "SystemFunctions.h"
#include "SystemMemory.h"

#define DIRECTX12_MAX_GRAPHICSHEAP 32
#define DIRECTX12_MAX_TEXTURES UINT16_MAX

SystemDataPool<DirectX12GraphicsHeapData, DirectX12GraphicsHeapDataFull> directX12GraphicsHeapPool;
SystemDataPool<DirectX12TextureData, DirectX12TextureDataFull> directX12TexturePool;

void InitDirectX12ResourceMemory()
{
    if (!directX12GraphicsHeapPool.Storage)
    {
        directX12GraphicsHeapPool = SystemCreateDataPool<DirectX12GraphicsHeapData, DirectX12GraphicsHeapDataFull>(DirectX12MemoryArena, DIRECTX12_MAX_GRAPHICSHEAP);
        directX12TexturePool = SystemCreateDataPool<DirectX12TextureData, DirectX12TextureDataFull>(DirectX12MemoryArena, DIRECTX12_MAX_TEXTURES);
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

DirectX12TextureData* GetDirectX12TextureData(ElemTexture texture)
{
    return SystemGetDataPoolItem(directX12TexturePool, texture);
}

DirectX12TextureDataFull* GetDirectX12TextureDataFull(ElemTexture texture)
{
    return SystemGetDataPoolItemFull(directX12TexturePool, texture);
}

DXGI_FORMAT ConvertDirectX12FormatToSrgbIfNeeded(DXGI_FORMAT format)
{
    switch (format) 
    {
        case DXGI_FORMAT_B8G8R8A8_UNORM:
            return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;

        default:
            return format;
    }
}

ElemTexture CreateDirectX12TextureFromResource(ElemGraphicsDevice graphicsDevice, ComPtr<ID3D12Resource> resource, bool isPresentTexture)
{
    InitDirectX12ResourceMemory();

    auto graphicsDeviceData = GetDirectX12GraphicsDeviceData(graphicsDevice);
    SystemAssert(graphicsDeviceData);

    auto resourceDesc = resource->GetDesc();
    D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptor = {};

    if (resourceDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET)
    {
        // TODO: Do we create the RTV view here?
        rtvDescriptor = CreateDirectX12DescriptorHandle(graphicsDeviceData->RTVDescriptorHeap);

        D3D12_RENDER_TARGET_VIEW_DESC renderTargetViewDesc = 
        {
            .Format = ConvertDirectX12FormatToSrgbIfNeeded(resourceDesc.Format),
            .ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D,
        };
 
        graphicsDeviceData->Device->CreateRenderTargetView(resource.Get(), &renderTargetViewDesc, rtvDescriptor);
    }

    auto handle = SystemAddDataPoolItem(directX12TexturePool, {
        .DeviceObject = resource,
        .ResourceDescription = resourceDesc,
        .IsPresentTexture = isPresentTexture,
        .RtvDescriptor = rtvDescriptor
    }); 

    SystemAddDataPoolItemFull(directX12TexturePool, handle, {
        .GraphicsDevice = graphicsDevice
    });

    return handle;
}

DXGI_FORMAT ConvertToDirectX12TextureFormat(ElemTextureFormat format)
{
    switch (format) 
    {
        case ElemTextureFormat_B8G8R8A8_SRGB:
            return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;

        case ElemTextureFormat_B8G8R8A8_UNORM:
            return DXGI_FORMAT_B8G8R8A8_UNORM;

        case ElemTextureFormat_R16G16B16A16_FLOAT:
            return DXGI_FORMAT_R16G16B16A16_FLOAT;
    }
}

D3D12_RESOURCE_FLAGS ConvertToDirectX12ResourceFlags(ElemTextureUsage usage)
{
    switch (usage) 
    {
        case ElemTextureUsage_Standard:
            return D3D12_RESOURCE_FLAG_NONE;

        case ElemTextureUsage_Uav:
            return D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

        case ElemTextureUsage_RenderTarget:
            return D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    }
}

D3D12_RESOURCE_DESC1 CreateDirectX12TextureDescription(const ElemTextureParameters* parameters)
{
    SystemAssert(parameters);
    
	return 
    {
        .Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
        .Alignment = 0,
        .Width = parameters->Width,
        .Height = parameters->Height,
        .DepthOrArraySize = 1,
        .MipLevels = 1,
        .Format = ConvertToDirectX12TextureFormat(parameters->Format),
        .SampleDesc =
        {
            .Count = 1,
            .Quality = 0
        },
        .Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
        .Flags = ConvertToDirectX12ResourceFlags(parameters->Usage)
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

    graphicsHeapData->DeviceObject.Reset();
}

void DirectX12BindGraphicsHeap(ElemCommandList commandList, ElemGraphicsHeap graphicsHeap)
{
}


ElemTexture DirectX12CreateTexture(ElemGraphicsHeap graphicsHeap, uint64_t graphicsHeapOffset, const ElemTextureParameters* parameters)
{
    auto stackMemoryArena = SystemGetStackMemoryArena();
    SystemAssert(graphicsHeap != ELEM_HANDLE_NULL);

    auto graphicsHeapData = GetDirectX12GraphicsHeapData(graphicsHeap);
    SystemAssert(graphicsHeapData);

    auto graphicsDeviceData = GetDirectX12GraphicsDeviceData(graphicsHeapData->GraphicsDevice);
    SystemAssert(graphicsDeviceData);

    auto resourceDescription = CreateDirectX12TextureDescription(parameters);

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

    if (DirectX12DebugLayerEnabled && parameters->DebugName)
    {
        texture->SetName(SystemConvertUtf8ToWideChar(stackMemoryArena, parameters->DebugName).Pointer);
    }

    return CreateDirectX12TextureFromResource(graphicsHeapData->GraphicsDevice, texture, false);
}

// TODO: Functions to create additional optional Descriptors

void DirectX12FreeTexture(ElemTexture texture)
{
    // TODO: Do a kind a deferred texture delete so we don't crash if the resource is still in use
    SystemAssert(texture != ELEM_HANDLE_NULL);

    auto textureData = GetDirectX12TextureData(texture);
    SystemAssert(textureData);

    auto textureDataFull = GetDirectX12TextureDataFull(texture);
    SystemAssert(textureDataFull);

    auto graphicsDeviceData = GetDirectX12GraphicsDeviceData(textureDataFull->GraphicsDevice);
    SystemAssert(graphicsDeviceData);

    // TODO: Remove descriptor handles
    if (textureData->RtvDescriptor.ptr)
    {
        FreeDirectX12DescriptorHandle(graphicsDeviceData->RTVDescriptorHeap, textureData->RtvDescriptor);
        textureData->RtvDescriptor.ptr = 0;
    }

    if (textureData->DeviceObject)
    {
        textureData->DeviceObject.Reset();
    }

    SystemRemoveDataPoolItem(directX12TexturePool, texture);
}

ElemShaderDescriptor DirectX12CreateTextureShaderDescriptor(ElemTexture texture, const ElemTextureShaderDescriptorOptions* options)
{
    SystemAssert(texture != ELEM_HANDLE_NULL);

    auto textureData = GetDirectX12TextureData(texture);
    SystemAssert(textureData);

    auto textureDataFull = GetDirectX12TextureDataFull(texture);
    SystemAssert(textureDataFull);

    auto graphicsDeviceData = GetDirectX12GraphicsDeviceData(textureDataFull->GraphicsDevice);
    SystemAssert(graphicsDeviceData);

    DirectX12DescriptorHeap descriptorHeap;

    if (options->Type == ElemTextureShaderDescriptorType_RenderTarget)
    {
        descriptorHeap = graphicsDeviceData->RTVDescriptorHeap;
    }
    else 
    {
        descriptorHeap = graphicsDeviceData->ResourceDescriptorHeap;
    }
        
    auto descriptorHandle = CreateDirectX12DescriptorHandle(graphicsDeviceData->ResourceDescriptorHeap);

    switch (options->Type)
    {
        case ElemTextureShaderDescriptorType_Read:
        {
            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = 
            {
                .Format = textureData->ResourceDescription.Format,
                .ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
                .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
                .Texture2D =
                {
                    .MostDetailedMip = options->MipIndex,
                    .MipLevels = textureData->ResourceDescription.MipLevels - options->MipIndex
                }
            };

		    graphicsDeviceData->Device->CreateShaderResourceView(textureData->DeviceObject.Get(), &srvDesc, descriptorHandle);
            break;
        }
        case ElemTextureShaderDescriptorType_Uav:
        {
            D3D12_UNORDERED_ACCESS_VIEW_DESC uavViewDesc = 
            {
                .Format = textureData->ResourceDescription.Format,
                .ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D,
                .Texture2D = 
                {
                    .MipSlice = options->MipIndex
                }
            };
            
            graphicsDeviceData->Device->CreateUnorderedAccessView(textureData->DeviceObject.Get(), nullptr, &uavViewDesc, descriptorHandle);
            break;
        }
        case ElemTextureShaderDescriptorType_RenderTarget:
        {
            D3D12_RENDER_TARGET_VIEW_DESC renderTargetViewDesc = 
            {
                .Format = ConvertDirectX12FormatToSrgbIfNeeded(textureData->ResourceDescription.Format),
                .ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D,
            };
     
            graphicsDeviceData->Device->CreateRenderTargetView(textureData->DeviceObject.Get(), &renderTargetViewDesc, descriptorHandle);
            break;
        }
    }

    return ConvertDirectX12DescriptorHandleToIndex(descriptorHeap, descriptorHandle);
}

void DirectX12FreeShaderDescriptor(ElemShaderDescriptor shaderDescriptor)
{
}
