#include "DirectX12Resource.h"
#include "DirectX12Config.h"
#include "DirectX12GraphicsDevice.h"
#include "DirectX12CommandList.h"
#include "DirectX12ResourceBarrier.h"
#include "Graphics/Resource.h"
#include "Graphics/ResourceDeleteQueue.h"
#include "Graphics/UploadBufferPool.h"
#include "SystemDataPool.h"
#include "SystemFunctions.h"
#include "SystemMemory.h"

// TODO: Implement: https://github.com/microsoft/DirectX-Specs/blob/master/d3d/D3D12TightPlacedResourceAlignment.md


SystemDataPool<DirectX12GraphicsHeapData, DirectX12GraphicsHeapDataFull> directX12GraphicsHeapPool;
SystemDataPool<DirectX12GraphicsResourceData, DirectX12GraphicsResourceDataFull> directX12GraphicsResourcePool;

thread_local UploadBufferDevicePool<ComPtr<ID3D12Resource>> threadDirectX12UploadBufferPools[DIRECTX12_MAX_DEVICES];

// TODO: Can we get rid of those???
Span<ElemGraphicsResourceDescriptorInfo> directX12ResourceDescriptorInfos;
Span<ElemGraphicsSamplerInfo> directX12SamplerInfos;

MemoryArena directX12RaytracingInstanceMemoryArena;
MemoryArena directX12ReadBackMemoryArena;

// TODO: Reorder the functions to match the header

void InitDirectX12ResourceMemory()
{
    if (!directX12GraphicsHeapPool.Storage)
    {
        directX12GraphicsHeapPool = SystemCreateDataPool<DirectX12GraphicsHeapData, DirectX12GraphicsHeapDataFull>(DirectX12MemoryArena, DIRECTX12_MAX_GRAPHICSHEAP);
        directX12GraphicsResourcePool = SystemCreateDataPool<DirectX12GraphicsResourceData, DirectX12GraphicsResourceDataFull>(DirectX12MemoryArena, DIRECTX12_MAX_RESOURCES);

        directX12ResourceDescriptorInfos = SystemPushArray<ElemGraphicsResourceDescriptorInfo>(DirectX12MemoryArena, DIRECTX12_MAX_RESOURCES, AllocationState_Reserved);
        directX12SamplerInfos = SystemPushArray<ElemGraphicsSamplerInfo>(DirectX12MemoryArena, DIRECTX12_MAX_SAMPLERS, AllocationState_Reserved);

        // TODO: Allow to increase the size as a parameter
        directX12RaytracingInstanceMemoryArena = SystemAllocateMemoryArena(DIRECTX12_RAYTRACING_INSTANCE_MEMORY_ARENA);
        directX12ReadBackMemoryArena = SystemAllocateMemoryArena(DIRECTX12_READBACK_MEMORY_ARENA);
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

D3D12_FILTER_TYPE ConvertToDirectX12FilterType(ElemGraphicsSamplerFilter filter)
{
    switch (filter)
    {
        case ElemGraphicsSamplerFilter_Nearest:
            return D3D12_FILTER_TYPE_POINT;

        case ElemGraphicsSamplerFilter_Linear:
            return D3D12_FILTER_TYPE_LINEAR;
    }
}

D3D12_TEXTURE_ADDRESS_MODE ConvertToDirectX12TextureAddressMode(ElemGraphicsSamplerAddressMode addressMode)
{
    switch (addressMode)
    {
        case ElemGraphicsSamplerAddressMode_Repeat:
            return D3D12_TEXTURE_ADDRESS_MODE_WRAP;

        case ElemGraphicsSamplerAddressMode_RepeatMirror:
            return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;

        case ElemGraphicsSamplerAddressMode_ClampToEdge:
            return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;

        case ElemGraphicsSamplerAddressMode_ClampToEdgeMirror:
            return D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;

        case ElemGraphicsSamplerAddressMode_ClampToBorderColor:
            return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    }
}

ElemGraphicsResource CreateDirectX12GraphicsResourceFromResource(ElemGraphicsDevice graphicsDevice, ElemGraphicsResourceType type, ElemGraphicsHeap heap, ComPtr<ID3D12Resource> resource, bool isPresentTexture, bool convertToSrgb)
{
    InitDirectX12ResourceMemory();

    auto graphicsDeviceData = GetDirectX12GraphicsDeviceData(graphicsDevice);
    SystemAssert(graphicsDeviceData);

    auto resourceDesc = resource->GetDesc();
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = {};
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = {};

    if (resourceDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET)
    {
        rtvHandle = CreateDirectX12DescriptorHandle(graphicsDeviceData->RTVDescriptorHeap);
        
        D3D12_RENDER_TARGET_VIEW_DESC renderTargetViewDesc = 
        {
            .Format = convertToSrgb ? ConvertDirectX12FormatToSrgbIfNeeded(resourceDesc.Format) : resourceDesc.Format,
            .ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D,
        };
 
        graphicsDeviceData->Device->CreateRenderTargetView(resource.Get(), &renderTargetViewDesc, rtvHandle);
    }
    else if (resourceDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)
    {
        dsvHandle = CreateDirectX12DescriptorHandle(graphicsDeviceData->DSVDescriptorHeap);
        
        D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc = 
        {
            .Format = ConvertDirectX12FormatToSrgbIfNeeded(resourceDesc.Format),
            .ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D,
        };
 
        graphicsDeviceData->Device->CreateDepthStencilView(resource.Get(), &depthStencilViewDesc, dsvHandle);
    }
  
    auto handle = SystemAddDataPoolItem(directX12GraphicsResourcePool, {
        .DeviceObject = resource,
        .RtvHandle = rtvHandle,
        .DsvHandle = dsvHandle,
        .Type = type,
        .DirectX12Format = resourceDesc.Format,
        .DirectX12Flags = resourceDesc.Flags,
        .GraphicsHeap = heap,
        .Width = (uint32_t)resourceDesc.Width,
        .Height = type != ElemGraphicsResourceType_Buffer ? (uint32_t)resourceDesc.Height : 0,
        .MipLevels = resourceDesc.MipLevels,
        .IsPresentTexture = isPresentTexture
    }); 

    SystemAddDataPoolItemFull(directX12GraphicsResourcePool, handle, {
        .GraphicsDevice = graphicsDevice,
    });

    return handle;
}

DXGI_FORMAT ConvertToDirectX12Format(ElemGraphicsFormat format)
{
    switch (format) 
    {
        case ElemGraphicsFormat_B8G8R8A8_SRGB:
            return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;

        case ElemGraphicsFormat_B8G8R8A8:
            return DXGI_FORMAT_B8G8R8A8_UNORM;

        case ElemGraphicsFormat_R16G16B16A16_FLOAT:
            return DXGI_FORMAT_R16G16B16A16_FLOAT;

        case ElemGraphicsFormat_R32G32B32A32_FLOAT:
            return DXGI_FORMAT_R32G32B32A32_FLOAT;

        case ElemGraphicsFormat_R32G32B32_FLOAT:
            return DXGI_FORMAT_R32G32B32_FLOAT;

        case ElemGraphicsFormat_R32_UINT:
            return DXGI_FORMAT_R32_UINT;

        case ElemGraphicsFormat_D32_FLOAT:
            return DXGI_FORMAT_D32_FLOAT;

        case ElemGraphicsFormat_BC7:
            return DXGI_FORMAT_BC7_UNORM;

        case ElemGraphicsFormat_BC7_SRGB:
            return DXGI_FORMAT_BC7_UNORM_SRGB;

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
            return ElemGraphicsFormat_B8G8R8A8;

        case DXGI_FORMAT_R16G16B16A16_FLOAT:
            return ElemGraphicsFormat_R16G16B16A16_FLOAT;

        case DXGI_FORMAT_R32G32B32A32_FLOAT:
            return ElemGraphicsFormat_R32G32B32A32_FLOAT;

        case DXGI_FORMAT_D32_FLOAT:
            return ElemGraphicsFormat_D32_FLOAT;

        case DXGI_FORMAT_BC7_UNORM:
            return ElemGraphicsFormat_BC7;

        case DXGI_FORMAT_BC7_UNORM_SRGB:
            return ElemGraphicsFormat_BC7_SRGB;

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
    
    if (usage & ElemGraphicsResourceUsage_DepthStencil)
    {
        result |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    }
    
    if (usage & ElemGraphicsResourceUsage_RaytracingAccelerationStructure)
    {
        result |= D3D12_RESOURCE_FLAG_RAYTRACING_ACCELERATION_STRUCTURE;
        result |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
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
    
    if (flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)
    {
        result = (ElemGraphicsResourceUsage)(result | ElemGraphicsResourceUsage_DepthStencil);
    }
    
    if (flags & D3D12_RESOURCE_FLAG_RAYTRACING_ACCELERATION_STRUCTURE)
    {
        result = (ElemGraphicsResourceUsage)(result | ElemGraphicsResourceUsage_RaytracingAccelerationStructure);
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
        .Format = ConvertToDirectX12Format(resourceInfo->Format),
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
    SystemAssert(resourceInfo);
    
	return 
    {
        .Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
        .Alignment = 0,
        .Width = resourceInfo->Width,
        .Height = resourceInfo->Height,
        .DepthOrArraySize = 1,
        .MipLevels = (uint16_t)resourceInfo->MipLevels,
        .Format = ConvertToDirectX12Format(resourceInfo->Format),
        .SampleDesc =
        {
            .Count = 1,
            .Quality = 0
        },
        .Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
        .Flags = ConvertToDirectX12ResourceFlags(resourceInfo->Usage)
    };
}

D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS ConvertToDirectX12RaytracingBuildFlags(ElemRaytracingBuildFlags buildFlags)
{
    auto result = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;

    if (buildFlags & ElemRaytracingBuildFlags_AllowUpdate)
    {
        result |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
    }

    if (buildFlags & ElemRaytracingBuildFlags_AllowCompaction)
    {
        result |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_COMPACTION;
    }
    
    if (buildFlags & ElemRaytracingBuildFlags_PreferFastTrace)
    {
        result |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
    }

    if (buildFlags & ElemRaytracingBuildFlags_PreferFastBuild)
    {
        result |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD;
    }

    if (buildFlags & ElemRaytracingBuildFlags_MinimizeMemory)
    {
        result |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_MINIMIZE_MEMORY;
    }

    return result;
}

D3D12_RAYTRACING_INSTANCE_FLAGS ConvertToDirectX12RaytracingInstanceFlags(ElemRaytracingTlasInstanceFlags instanceFlags)
{
    auto result = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;

    if (instanceFlags & ElemRaytracingTlasInstanceFlags_DisableTriangleCulling)
    {
        result |= D3D12_RAYTRACING_INSTANCE_FLAG_TRIANGLE_CULL_DISABLE;
    }

    if (instanceFlags & ElemRaytracingTlasInstanceFlags_FlipTriangleFaces)
    {
        result |= D3D12_RAYTRACING_INSTANCE_FLAG_TRIANGLE_FRONT_COUNTERCLOCKWISE;
    }

    if (instanceFlags & ElemRaytracingTlasInstanceFlags_NonOpaque)
    {
        result |= D3D12_RAYTRACING_INSTANCE_FLAG_FORCE_NON_OPAQUE;
    }

    return result;
}

DXGI_FORMAT ConvertRaytracingIndexFormatToDirectX12Format(ElemRaytracingIndexFormat format)
{
    switch (format) 
    {
        case ElemRaytracingIndexFormat_UInt32:
            return DXGI_FORMAT_R32_UINT;

        case ElemRaytracingIndexFormat_UInt16:
            return DXGI_FORMAT_R16_UINT;
    }
}

DXGI_FORMAT ConvertRaytracingVertexFormatToDirectX12Format(ElemRaytracingVertexFormat format)
{
    switch (format) 
    {
        case ElemRaytracingVertexFormat_Float32:
            return DXGI_FORMAT_R32G32B32_FLOAT;

        case ElemRaytracingVertexFormat_Float16:
            return DXGI_FORMAT_R16G16B16A16_FLOAT ;
    }
}

D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS BuildDirectX12BlasInputs(MemoryArena memoryArena, const ElemRaytracingBlasParameters* parameters)
{
    SystemAssert(parameters);
    auto geometryList = SystemPushArray<D3D12_RAYTRACING_GEOMETRY_DESC>(memoryArena, parameters->GeometryList.Length);

    for (uint32_t i = 0; i < parameters->GeometryList.Length; i++)
    {
        auto geometryDesc = &parameters->GeometryList.Items[i];
        auto geometry = &geometryList[i];

        *geometry =
        {
            .Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES,
            .Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE,
            .Triangles = 
            {
                .IndexFormat = ConvertRaytracingIndexFormatToDirectX12Format(geometryDesc->IndexFormat),
                .VertexFormat = ConvertRaytracingVertexFormatToDirectX12Format(geometryDesc->VertexFormat),
                .IndexCount = geometryDesc->IndexCount,
                .VertexCount = geometryDesc->VertexCount,
                .VertexBuffer = 
                {
                    .StrideInBytes = geometryDesc->VertexSizeInBytes
                }
            }
        };

        if (geometryDesc->VertexBuffer != ELEM_HANDLE_NULL)
        {
            auto vertexBufferResourceData = GetDirectX12GraphicsResourceData(geometryDesc->VertexBuffer);
            SystemAssert(vertexBufferResourceData);

            geometry->Triangles.VertexBuffer.StartAddress = vertexBufferResourceData->DeviceObject->GetGPUVirtualAddress() + geometryDesc->VertexBufferOffset;
        }

        if (geometryDesc->IndexBuffer != ELEM_HANDLE_NULL)
        {
            auto indexBufferResourceData = GetDirectX12GraphicsResourceData(geometryDesc->IndexBuffer);
            SystemAssert(indexBufferResourceData);

            geometry->Triangles.IndexBuffer = indexBufferResourceData->DeviceObject->GetGPUVirtualAddress() + geometryDesc->IndexBufferOffset;
        }
    }

    return
    {
        .Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL,
        .Flags = ConvertToDirectX12RaytracingBuildFlags(parameters->BuildFlags), 
        .NumDescs = (uint32_t)geometryList.Length,
        .DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY,
        .pGeometryDescs = geometryList.Pointer
    };
}

D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS BuildDirectX12TlasInputs(MemoryArena memoryArena, const ElemRaytracingTlasParameters* parameters)
{
    SystemAssert(parameters);
    
    DirectX12GraphicsResourceData* instanceBufferResourceData = nullptr;
    
    if (parameters->InstanceBuffer)
    {
        instanceBufferResourceData = GetDirectX12GraphicsResourceData(parameters->InstanceBuffer);
        SystemAssert(instanceBufferResourceData);
    }

    return
    {
        .Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL,
        .Flags = ConvertToDirectX12RaytracingBuildFlags(parameters->BuildFlags), 
        .NumDescs = parameters->InstanceCount,
        .DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY,
        .InstanceDescs = parameters->InstanceBuffer ? instanceBufferResourceData->DeviceObject->GetGPUVirtualAddress() + parameters->InstanceBufferOffset : 0
    };
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
        .Type = D3D12_HEAP_TYPE_DEFAULT
    };

    if (options)
    {
        if (options->HeapType == ElemGraphicsHeapType_GpuUpload)
        {
            heapProperties.Type = D3D12_HEAP_TYPE_GPU_UPLOAD;
        }
        else if (options->HeapType == ElemGraphicsHeapType_Readback)
        {
            heapProperties = graphicsDeviceData->Device->GetCustomHeapProperties(0, D3D12_HEAP_TYPE_READBACK);
        }
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
        .HeapDescription = heapDesc,
        .HeapType = heapProperties.Type
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

    // TODO: Handle clear value
    D3D12_CLEAR_VALUE clearValue = 
    {
    };

    D3D12_CLEAR_VALUE* clearValuePointer = nullptr;

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

        resourceDescription = CreateDirectX12TextureDescription(resourceInfo);

        if (resourceInfo->Usage & ElemGraphicsResourceUsage_RenderTarget || resourceInfo->Usage & ElemGraphicsResourceUsage_DepthStencil)
        {
            clearValue.Format = resourceDescription.Format;
            clearValuePointer = &clearValue;
        }
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

        if (resourceInfo->Usage & ElemGraphicsResourceUsage_RaytracingAccelerationStructure && graphicsHeapData->HeapType != D3D12_HEAP_TYPE_DEFAULT)
        {
            SystemLogErrorMessage(ElemLogMessageCategory_Graphics, "GraphicsBuffer with usage RaytracingAccelerationStructure should be allocated on a Gpu Heap.");
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
                                                                     clearValuePointer, 
                                                                     0,
                                                                     nullptr,
                                                                     IID_PPV_ARGS(resource.GetAddressOf())));

    if (DirectX12DebugLayerEnabled && resourceInfo->DebugName)
    {
        resource->SetName(SystemConvertUtf8ToWideChar(stackMemoryArena, resourceInfo->DebugName).Pointer);
    }

    return CreateDirectX12GraphicsResourceFromResource(graphicsHeapData->GraphicsDevice, resourceInfo->Type, graphicsHeap, resource, false);
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

void DirectX12UploadGraphicsBufferData(ElemGraphicsResource buffer, uint32_t offset, ElemDataSpan data)
{
    SystemAssert(buffer != ELEM_HANDLE_NULL);

    auto resourceData = GetDirectX12GraphicsResourceData(buffer);
    SystemAssert(resourceData);

    auto heapData = GetDirectX12GraphicsHeapData(resourceData->GraphicsHeap);
    SystemAssert(heapData);

    if (resourceData->Type != ElemGraphicsResourceType_Buffer)
    {
        SystemLogErrorMessage(ElemLogMessageCategory_Graphics, "ElemUploadGraphicsBufferData only works with graphics buffers.");
        return;
    }

    if (heapData->HeapType != D3D12_HEAP_TYPE_GPU_UPLOAD)
    {
        SystemLogErrorMessage(ElemLogMessageCategory_Graphics, "ElemUploadGraphicsBufferData only works with graphics buffers allocated in a GpuUpload heap.");
        return;
    }

	if (resourceData->CpuDataPointer == nullptr)
	{
	    D3D12_RANGE readRange = { 0, 0 };
	    resourceData->DeviceObject->Map(0, &readRange, &resourceData->CpuDataPointer);
	}
    
    auto destinationPointer = (uint8_t*)resourceData->CpuDataPointer + offset;
    memcpy(destinationPointer, data.Items, data.Length);
}

ElemDataSpan DirectX12DownloadGraphicsBufferData(ElemGraphicsResource buffer, const ElemDownloadGraphicsBufferDataOptions* options)
{
    SystemAssert(buffer != ELEM_HANDLE_NULL);

    auto resourceData = GetDirectX12GraphicsResourceData(buffer);
    SystemAssert(resourceData);

    auto heapData = GetDirectX12GraphicsHeapData(resourceData->GraphicsHeap);
    SystemAssert(heapData);

    if (resourceData->Type != ElemGraphicsResourceType_Buffer)
    {
        SystemLogErrorMessage(ElemLogMessageCategory_Graphics, "ElemDownloadGraphicsBufferData only works with graphics buffers.");
        return {};
    }

    if (heapData->HeapType != D3D12_HEAP_TYPE_CUSTOM && heapData->HeapType != D3D12_HEAP_TYPE_GPU_UPLOAD)
    {
        SystemLogErrorMessage(ElemLogMessageCategory_Graphics, "ElemDownloadGraphicsBufferData only works with graphics buffers allocated in a Readback heap or GpuUpload heap. (%d)", heapData->HeapType);
        return {};
    }

    if (heapData->HeapType != D3D12_HEAP_TYPE_CUSTOM)
    {
        SystemLogWarningMessage(ElemLogMessageCategory_Graphics, "ElemDownloadGraphicsBufferData works faster with graphics buffers allocated in a Readback heap.");
    }

	if (resourceData->CpuDataPointer == nullptr)
	{
	    D3D12_RANGE readRange = { 0, 0 };
	    resourceData->DeviceObject->Map(0, &readRange, &resourceData->CpuDataPointer);
	}

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

    auto downloadedData = SystemPushArray<uint8_t>(directX12ReadBackMemoryArena, sizeInBytes);
    memcpy(downloadedData.Pointer, (uint8_t*)resourceData->CpuDataPointer + offset, sizeInBytes);

	return { .Items = downloadedData.Pointer, .Length = (uint32_t)downloadedData.Length };
}

ComPtr<ID3D12Resource> CreateDirectX12UploadBuffer(ComPtr<ID3D12Device10> graphicsDevice, uint64_t sizeInBytes)
{
    D3D12_RESOURCE_DESC bufferDescription =
    {
        .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
        .Alignment = 0,
        .Width = sizeInBytes,
        .Height = 1,
        .DepthOrArraySize = 1,
        .MipLevels = 1,
        .Format = DXGI_FORMAT_UNKNOWN,
        .SampleDesc =
        {
            .Count = 1,
            .Quality = 0
        },
        .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
        .Flags = D3D12_RESOURCE_FLAG_NONE
    };
    
    D3D12_HEAP_PROPERTIES heapProperties = { .Type = D3D12_HEAP_TYPE_UPLOAD };
    
    ComPtr<ID3D12Resource> resource;
    AssertIfFailed(graphicsDevice->CreateCommittedResource1(&heapProperties, 
                                                            D3D12_HEAP_FLAG_NONE, 
                                                            &bufferDescription, 
                                                            D3D12_RESOURCE_STATE_COMMON, 
                                                            nullptr, 
                                                            nullptr, 
                                                            IID_PPV_ARGS(resource.GetAddressOf())));

    resource->SetName(L"ElementalUploadBuffer");

    return resource;
}

UploadBufferMemory<ComPtr<ID3D12Resource>> GetDirectX12UploadBuffer(ElemGraphicsDevice graphicsDevice, uint64_t alignment, uint64_t sizeInBytes)
{
    auto graphicsDeviceData = GetDirectX12GraphicsDeviceData(graphicsDevice);
    SystemAssert(graphicsDeviceData);

    auto graphicsIdUnpacked = UnpackSystemDataPoolHandle(graphicsDevice);
    auto uploadBufferPool = &threadDirectX12UploadBufferPools[graphicsIdUnpacked.Index];

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
            DirectX12WaitForFenceOnCpu(uploadBuffer.PoolItem->Fence);
        }
        
        if (uploadBuffer.PoolItem->Buffer != nullptr)
        {
            SystemLogDebugMessage(ElemLogMessageCategory_Graphics, "Need to delete buffer");
            uploadBuffer.PoolItem->Buffer.Reset();
        }

        uploadBuffer.PoolItem->Buffer = CreateDirectX12UploadBuffer(graphicsDeviceData->Device, uploadBuffer.PoolItem->SizeInBytes);

	    D3D12_RANGE readRange = { 0, 0 };
	    uploadBuffer.PoolItem->Buffer->Map(0, &readRange, (void**)&uploadBuffer.PoolItem->CpuPointer);

        uploadBuffer.PoolItem->IsResetNeeded = false;
    }

    return uploadBuffer;
}

void DirectX12CopyDataToGraphicsResource(ElemCommandList commandList, const ElemCopyDataToGraphicsResourceParameters* parameters)
{
    // TODO: Implement optimizations on the copy queue. On windows, use DirectStorage
    auto stackMemoryArena = SystemGetStackMemoryArena();

    SystemAssert(commandList != ELEM_HANDLE_NULL);

    SystemAssert(parameters);
    SystemAssert(parameters->Resource != ELEM_HANDLE_NULL);

    auto commandListData = GetDirectX12CommandListData(commandList);
    SystemAssert(commandListData);

    auto graphicsDeviceData = GetDirectX12GraphicsDeviceData(commandListData->GraphicsDevice);
    SystemAssert(graphicsDeviceData);

    auto resourceData = GetDirectX12GraphicsResourceData(parameters->Resource);
    SystemAssert(resourceData);

    auto resourceDataFull = GetDirectX12GraphicsResourceDataFull(parameters->Resource);
    SystemAssert(resourceDataFull);

    ReadOnlySpan<uint8_t> sourceData;

    // TODO: Implement file source
    if (parameters->SourceType == ElemCopyDataSourceType_Memory)
    {
        sourceData = ReadOnlySpan<uint8_t>(parameters->SourceMemoryData.Items, parameters->SourceMemoryData.Length);
    }
    
    auto uploadBufferAlignment = 4u;
    auto uploadBufferSizeInBytes = sourceData.Length;
    DirectX12GraphicsTextureMipCopyInfo textureMipCopyInfo = {};

    if (resourceData->Type == ElemGraphicsResourceType_Texture2D)
    {
        uploadBufferAlignment = D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT;
        auto resourceDesc = resourceData->DeviceObject->GetDesc();

        graphicsDeviceData->Device->GetCopyableFootprints(&resourceDesc, 
                                                          parameters->TextureMipLevel, 1, 0, 
                                                          &textureMipCopyInfo.PlacedFootprint, 
                                                          &textureMipCopyInfo.RowCount, 
                                                          &textureMipCopyInfo.SourceRowSizeInBytes, 
                                                          &uploadBufferSizeInBytes);
    }

    auto uploadBuffer = GetDirectX12UploadBuffer(commandListData->GraphicsDevice, uploadBufferAlignment, uploadBufferSizeInBytes);
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

    if (resourceData->Type == ElemGraphicsResourceType_Buffer)
    {
        memcpy(uploadBuffer.PoolItem->CpuPointer + uploadBuffer.Offset, sourceData.Pointer, sourceData.Length);
        commandListData->DeviceObject->CopyBufferRegion(resourceData->DeviceObject.Get(), parameters->BufferOffset, uploadBuffer.PoolItem->Buffer.Get(), uploadBuffer.Offset, sourceData.Length);
    }
    else if (resourceData->Type == ElemGraphicsResourceType_Texture2D)
    {
        auto placedFootprint = textureMipCopyInfo.PlacedFootprint;
        auto sourceRowSizeInBytes = textureMipCopyInfo.SourceRowSizeInBytes;

        auto destData = uploadBuffer.PoolItem->CpuPointer + uploadBuffer.Offset;

        for (uint32_t i = 0; i < textureMipCopyInfo.RowCount; i++)
        {
            auto uploadBufferRowData = destData + i * placedFootprint.Footprint.RowPitch;
            auto sourceRowData = sourceData.Pointer + i * sourceRowSizeInBytes;

            memcpy(uploadBufferRowData, sourceRowData, sourceRowSizeInBytes);
        }

        placedFootprint.Offset = uploadBuffer.Offset;

        D3D12_TEXTURE_COPY_LOCATION sourceLocation = 
        {
            .pResource = uploadBuffer.PoolItem->Buffer.Get(),
            .Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT,
            .PlacedFootprint = placedFootprint
        };

        D3D12_TEXTURE_COPY_LOCATION destinationLocation = 
        {
            .pResource = resourceData->DeviceObject.Get(),
            .Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
            .SubresourceIndex = parameters->TextureMipLevel
        };

        commandListData->DeviceObject->CopyTextureRegion(&destinationLocation, 0, 0, 0, &sourceLocation, nullptr);
    }
}

ElemGraphicsResourceDescriptor DirectX12CreateGraphicsResourceDescriptor(ElemGraphicsResource resource, ElemGraphicsResourceDescriptorUsage usage, const ElemGraphicsResourceDescriptorOptions* options)
{
    SystemAssert(resource != ELEM_HANDLE_NULL);

    auto resourceData = GetDirectX12GraphicsResourceData(resource);
    SystemAssert(resourceData);

    auto resourceUsage = ConvertFromDirectX12ResourceFlags(resourceData->DirectX12Flags);

    if (resourceData->Type == ElemGraphicsResourceType_Texture2D)
    {
        if (usage == ElemGraphicsResourceDescriptorUsage_Write && (resourceUsage & ElemGraphicsResourceUsage_Write) == 0)
        {
            SystemLogErrorMessage(ElemLogMessageCategory_Graphics, "Resource Descriptor write only works with texture created with write usage.");
            return -1;
        }
    }
    else
    {
        if (usage == ElemGraphicsResourceDescriptorUsage_Write && (resourceUsage & ElemGraphicsResourceUsage_Write) == 0)
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
            // TODO: Make a function to handle depth stencil formats checks
            .Format = resourceData->DirectX12Format == DXGI_FORMAT_D32_FLOAT ? DXGI_FORMAT_R32_FLOAT : resourceData->DirectX12Format,
            .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
        };

        if (resourceData->Type == ElemGraphicsResourceType_RaytracingAccelerationStructure)
        {
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE,
            srvDesc.RaytracingAccelerationStructure.Location = resourceData->DeviceObject->GetGPUVirtualAddress() + resourceData->SubResourceOffset;

            graphicsDeviceData->Device->CreateShaderResourceView(nullptr, &srvDesc, descriptorHandle);
        }
        else
        {
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

    auto index = ConvertDirectX12DescriptorHandleToIndex(descriptorHeap, descriptorHandle);

    if ((index % 1024) == 0)
    {
        SystemCommitMemory(DirectX12MemoryArena, &directX12ResourceDescriptorInfos[index], 1024 *  sizeof(ElemGraphicsResourceDescriptorInfo));
    }

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

void DirectX12ProcessGraphicsResourceDeleteQueue(ElemGraphicsDevice graphicsDevice)
{
    auto stackMemoryArena = SystemGetStackMemoryArena();

    ProcessResourceDeleteQueue();
    SystemClearMemoryArena(directX12RaytracingInstanceMemoryArena);
    SystemClearMemoryArena(directX12ReadBackMemoryArena);
    
    auto graphicsDeviceData = GetDirectX12GraphicsDeviceData(graphicsDevice);
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

                uploadBufferToDelete->Buffer.Reset();
                uploadBufferToDelete->CurrentOffset = 0;
                uploadBufferToDelete->SizeInBytes = 0;
            }
        }
    }
}

ElemGraphicsSampler DirectX12CreateGraphicsSampler(ElemGraphicsDevice graphicsDevice, const ElemGraphicsSamplerInfo* samplerInfo)
{
    InitDirectX12ResourceMemory();

    auto graphicsDeviceData = GetDirectX12GraphicsDeviceData(graphicsDevice);
    SystemAssert(graphicsDeviceData);

    auto descriptorHeap = graphicsDeviceData->SamplerDescriptorHeap;
    auto descriptorHandle = CreateDirectX12DescriptorHandle(descriptorHeap);

    auto localSamplerInfo = *samplerInfo;

    if (localSamplerInfo.MaxAnisotropy == 0)
    {
        localSamplerInfo.MaxAnisotropy = 1;
    }

    auto minFilter = ConvertToDirectX12FilterType(localSamplerInfo.MinFilter);
    auto magFilter = ConvertToDirectX12FilterType(localSamplerInfo.MagFilter);
    auto mipFilter = ConvertToDirectX12FilterType(localSamplerInfo.MipFilter);
    auto reduction = localSamplerInfo.CompareFunction == ElemGraphicsCompareFunction_Never ? D3D12_FILTER_REDUCTION_TYPE_STANDARD : D3D12_FILTER_REDUCTION_TYPE_COMPARISON;
    auto borderColor = localSamplerInfo.BorderColor;
    auto filter = D3D12_ENCODE_BASIC_FILTER(minFilter, magFilter, mipFilter, reduction);

    if (localSamplerInfo.MaxAnisotropy > 1)
    {
        filter = (D3D12_FILTER)(D3D12_ANISOTROPIC_FILTERING_BIT | filter);
    }

    D3D12_SAMPLER_DESC samplerDesc =
    {
        .Filter = filter,
        .AddressU = ConvertToDirectX12TextureAddressMode(localSamplerInfo.AddressU),
        .AddressV = ConvertToDirectX12TextureAddressMode(localSamplerInfo.AddressV),
        .AddressW = ConvertToDirectX12TextureAddressMode(localSamplerInfo.AddressW),
        .MaxAnisotropy = localSamplerInfo.MaxAnisotropy,
        .ComparisonFunc = ConvertToDirectX12CompareFunction(localSamplerInfo.CompareFunction),
        .BorderColor = { borderColor.Red, borderColor.Green, borderColor.Blue, borderColor.Alpha },
        .MinLOD = localSamplerInfo.MinLod,
        .MaxLOD = localSamplerInfo.MaxLod == 0 ? 1000 : localSamplerInfo.MaxLod
    };

    graphicsDeviceData->Device->CreateSampler(&samplerDesc, descriptorHandle);

    auto index = ConvertDirectX12DescriptorHandleToIndex(descriptorHeap, descriptorHandle);

    if ((index % 1024) == 0)
    {
        SystemCommitMemory(DirectX12MemoryArena, &directX12SamplerInfos[index], 1024 *  sizeof(ElemGraphicsSamplerInfo));
    }

    directX12SamplerInfos[index] = localSamplerInfo;
    return index;
}

ElemGraphicsSamplerInfo DirectX12GetGraphicsSamplerInfo(ElemGraphicsSampler sampler)
{
    InitDirectX12ResourceMemory();

    if (sampler == -1)
    {
        SystemLogErrorMessage(ElemLogMessageCategory_Graphics, "Sampler is invalid.");
        return {};
    }

    return directX12SamplerInfos[sampler];
}

void DirectX12FreeGraphicsSampler(ElemGraphicsSampler sampler, const ElemFreeGraphicsSamplerOptions* options)
{
    InitDirectX12ResourceMemory();

    if (sampler == -1)
    {
        SystemLogErrorMessage(ElemLogMessageCategory_Graphics, "Sampler is invalid.");
        return;
    }
    
    if (options && options->FencesToWait.Length > 0)
    {
        EnqueueResourceDeleteEntry(DirectX12MemoryArena, sampler, ResourceDeleteType_Sampler, options->FencesToWait);
        return;
    }

    directX12SamplerInfos[sampler] = {};
}

ElemRaytracingAllocationInfo DirectX12GetRaytracingBlasAllocationInfo(ElemGraphicsDevice graphicsDevice, const ElemRaytracingBlasParameters* parameters)
{
    auto stackMemoryArena = SystemGetStackMemoryArena();
    // TODO: Add validation
    
    auto graphicsDeviceData = GetDirectX12GraphicsDeviceData(graphicsDevice);
    SystemAssert(graphicsDeviceData);

    auto inputs = BuildDirectX12BlasInputs(stackMemoryArena, parameters);

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO allocationInfo;     
    graphicsDeviceData->Device->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &allocationInfo);

    return 
    {
        .Alignment = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT,
        .SizeInBytes = allocationInfo.ResultDataMaxSizeInBytes,
        .ScratchSizeInBytes = allocationInfo.ScratchDataSizeInBytes,
        .UpdateScratchSizeInBytes = allocationInfo.UpdateScratchDataSizeInBytes
    };
}

ElemRaytracingAllocationInfo DirectX12GetRaytracingTlasAllocationInfo(ElemGraphicsDevice graphicsDevice, const ElemRaytracingTlasParameters* parameters)
{
    auto stackMemoryArena = SystemGetStackMemoryArena();
    // TODO: Add validation
    
    auto graphicsDeviceData = GetDirectX12GraphicsDeviceData(graphicsDevice);
    SystemAssert(graphicsDeviceData);

    auto inputs = BuildDirectX12TlasInputs(stackMemoryArena, parameters);

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO allocationInfo;     
    graphicsDeviceData->Device->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &allocationInfo);

    return 
    {
        .Alignment = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT,
        .SizeInBytes = allocationInfo.ResultDataMaxSizeInBytes,
        .ScratchSizeInBytes = allocationInfo.ScratchDataSizeInBytes,
        .UpdateScratchSizeInBytes = allocationInfo.UpdateScratchDataSizeInBytes
    };
}

ElemGraphicsResourceAllocationInfo DirectX12GetRaytracingTlasInstanceAllocationInfo(ElemGraphicsDevice graphicsDevice, uint32_t instanceCount)
{
    return 
    {
        .Alignment = 4,
        .SizeInBytes = instanceCount * sizeof(D3D12_RAYTRACING_INSTANCE_DESC)
    };
}

ElemDataSpan DirectX12EncodeRaytracingTlasInstances(ElemRaytracingTlasInstanceSpan instances)
{
    InitDirectX12ResourceMemory();

    auto result = SystemPushArray<D3D12_RAYTRACING_INSTANCE_DESC>(directX12RaytracingInstanceMemoryArena, instances.Length);

    for (uint32_t i = 0; i < instances.Length; i++)
    {
        auto instance = &instances.Items[i];

        auto validVirtualAddress = false;
        uint64_t blasVirtualAddress = 0;

        if (instance->BlasResource)
        {
            auto blasResourceData = GetDirectX12GraphicsResourceData(instance->BlasResource);
            SystemAssert(blasResourceData);

            if (blasResourceData->Type == ElemGraphicsResourceType_RaytracingAccelerationStructure)
            {
                validVirtualAddress = true;
                blasVirtualAddress = blasResourceData->DeviceObject->GetGPUVirtualAddress() + blasResourceData->SubResourceOffset;
            }
        }

        if (!validVirtualAddress)
        {
            SystemLogErrorMessage(ElemLogMessageCategory_Graphics, "BlasResouce in Tlas instance should be an acceleration structure.");
            return {};
        }

        result[i] =
        {
            .InstanceID = instance->InstanceId,
            .InstanceMask = instance->InstanceMask,
            .Flags = (uint32_t)ConvertToDirectX12RaytracingInstanceFlags(instance->InstanceFlags),
            .AccelerationStructure =  blasVirtualAddress
        };
                
        for (uint32_t j = 0; j < 3; j++)
        {
            for (uint32_t k = 0; k < 4; k++)
            {
                result[i].Transform[j][k] = instance->TransformMatrix.Elements[k][j];
            }
        }
    }
    
    return { .Items = (uint8_t*)result.Pointer, .Length = (uint32_t)(result.Length * sizeof(D3D12_RAYTRACING_INSTANCE_DESC)) };
}

ElemGraphicsResource DirectX12CreateRaytracingAccelerationStructureResource(ElemGraphicsDevice graphicsDevice, ElemGraphicsResource storageBuffer, const ElemRaytracingAccelerationStructureOptions* options)
{
    auto resourceData = GetDirectX12GraphicsResourceData(storageBuffer);
    SystemAssert(resourceData);

    if (!(resourceData->DirectX12Flags & D3D12_RESOURCE_FLAG_RAYTRACING_ACCELERATION_STRUCTURE))
    {
        SystemLogErrorMessage(ElemLogMessageCategory_Graphics, "RaytracingAccelerationStructure need to have a storage buffer that was created with RaytracingAccelerationStructure usage.");
        return ELEM_HANDLE_NULL;
    }

    auto result = CreateDirectX12GraphicsResourceFromResource(graphicsDevice, ElemGraphicsResourceType_RaytracingAccelerationStructure, resourceData->GraphicsHeap, resourceData->DeviceObject, false);

    if (DirectX12DebugLayerEnabled && options && options->DebugName)
    {
        auto stackMemoryArena = SystemGetStackMemoryArena();
        resourceData->DeviceObject->SetName(SystemConvertUtf8ToWideChar(stackMemoryArena, options->DebugName).Pointer);
    }

    auto resultResourceData = GetDirectX12GraphicsResourceData(result);
    SystemAssert(resultResourceData);

    if (options)
    {
        resultResourceData->SubResourceOffset = options->StorageOffset;

        if (options->StorageSizeInBytes > 0)
        {
            resultResourceData->Width = options->StorageSizeInBytes;
        }
    }

    return result;
}

void DirectX12BuildRaytracingBlas(ElemCommandList commandList, ElemGraphicsResource accelerationStructure, ElemGraphicsResource scratchBuffer, const ElemRaytracingBlasParameters* parameters, const ElemRaytracingBuildOptions* options)
{
    auto stackMemoryArena = SystemGetStackMemoryArena();
    
    // TODO: Add validation

    auto commandListData = GetDirectX12CommandListData(commandList);
    SystemAssert(commandListData);

    auto graphicsDeviceData = GetDirectX12GraphicsDeviceData(commandListData->GraphicsDevice);
    SystemAssert(graphicsDeviceData);

    auto accelerationStructureResourceData = GetDirectX12GraphicsResourceData(accelerationStructure);
    SystemAssert(accelerationStructureResourceData);

    auto scratchBufferResourceData = GetDirectX12GraphicsResourceData(scratchBuffer);
    SystemAssert(scratchBufferResourceData);
    
    if (accelerationStructureResourceData->Type != ElemGraphicsResourceType_RaytracingAccelerationStructure)
    {
        SystemLogErrorMessage(ElemLogMessageCategory_Graphics, "Acceleration structure is not an acceleration structure graphics resource.");
        return;
    }
    
    auto inputs = BuildDirectX12BlasInputs(stackMemoryArena, parameters);

    auto scratchOffset = 0u;

    if (options)
    {
        scratchOffset = options->ScratchOffset;
    }

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC description 
    {
        .DestAccelerationStructureData = accelerationStructureResourceData->DeviceObject->GetGPUVirtualAddress() + accelerationStructureResourceData->SubResourceOffset,
        .Inputs = inputs,
        .ScratchAccelerationStructureData = scratchBufferResourceData->DeviceObject->GetGPUVirtualAddress() + scratchOffset
    };

    InsertDirectX12ResourceBarriersIfNeeded(commandList, ElemGraphicsResourceBarrierSyncType_BuildRaytracingAccelerationStructure);

    // TODO: Use the post structs?
    commandListData->DeviceObject->BuildRaytracingAccelerationStructure(&description, 0, nullptr);
}

void DirectX12BuildRaytracingTlas(ElemCommandList commandList, ElemGraphicsResource accelerationStructure, ElemGraphicsResource scratchBuffer, const ElemRaytracingTlasParameters* parameters, const ElemRaytracingBuildOptions* options)
{
    auto stackMemoryArena = SystemGetStackMemoryArena();
    
    // TODO: Add validation

    auto commandListData = GetDirectX12CommandListData(commandList);
    SystemAssert(commandListData);

    auto graphicsDeviceData = GetDirectX12GraphicsDeviceData(commandListData->GraphicsDevice);
    SystemAssert(graphicsDeviceData);

    auto accelerationStructureResourceData = GetDirectX12GraphicsResourceData(accelerationStructure);
    SystemAssert(accelerationStructureResourceData);

    auto scratchBufferResourceData = GetDirectX12GraphicsResourceData(scratchBuffer);
    SystemAssert(scratchBufferResourceData);
    
    auto inputs = BuildDirectX12TlasInputs(stackMemoryArena, parameters);

    auto scratchOffset = 0u;

    if (options)
    {
        scratchOffset = options->ScratchOffset;
    }

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC description 
    {
        .DestAccelerationStructureData = accelerationStructureResourceData->DeviceObject->GetGPUVirtualAddress() + accelerationStructureResourceData->SubResourceOffset,
        .Inputs = inputs,
        .ScratchAccelerationStructureData = scratchBufferResourceData->DeviceObject->GetGPUVirtualAddress() + scratchOffset
    };

    InsertDirectX12ResourceBarriersIfNeeded(commandList, ElemGraphicsResourceBarrierSyncType_BuildRaytracingAccelerationStructure);

    // TODO: Use the post structs?
    commandListData->DeviceObject->BuildRaytracingAccelerationStructure(&description, 0, nullptr);
}
