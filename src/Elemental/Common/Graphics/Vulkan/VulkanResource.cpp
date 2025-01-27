#include "VulkanResource.h"
#include "VulkanConfig.h"
#include "VulkanGraphicsDevice.h"
#include "VulkanCommandList.h"
#include "VulkanResourceBarrier.h"
#include "Graphics/Resource.h"
#include "Graphics/ResourceDeleteQueue.h"
#include "Graphics/UploadBufferPool.h"
#include "SystemDataPool.h"
#include "SystemFunctions.h"
#include "SystemMemory.h"

SystemDataPool<VulkanGraphicsHeapData, SystemDataPoolDefaultFull> vulkanGraphicsHeapPool;
SystemDataPool<VulkanGraphicsResourceData, VulkanGraphicsResourceDataFull> vulkanGraphicsResourcePool;

thread_local UploadBufferDevicePool<VulkanUploadBuffer> threadVulkanUploadBufferPools[VULKAN_MAX_DEVICES];

// TODO: IMPORTANT: This descriptor infos should be linked to the graphics device like the resource desc heaps
Span<ElemGraphicsResourceDescriptorInfo> vulkanResourceDescriptorInfos;
Span<VulkanGraphicsSamplerInfo> vulkanSamplerInfos;
// TODO: To refactor 
Span<VkImageView> vulkanResourceDescriptorImageViews;

MemoryArena vulkanRaytracingInstanceMemoryArena;
MemoryArena vulkanReadBackMemoryArena;

void InitVulkanResourceMemory()
{
    if (!vulkanGraphicsHeapPool.Storage)
    {
        vulkanGraphicsHeapPool = SystemCreateDataPool<VulkanGraphicsHeapData, SystemDataPoolDefaultFull>(VulkanGraphicsMemoryArena, VULKAN_MAX_GRAPHICSHEAP);
        vulkanGraphicsResourcePool = SystemCreateDataPool<VulkanGraphicsResourceData, VulkanGraphicsResourceDataFull>(VulkanGraphicsMemoryArena, VULKAN_MAX_RESOURCES);

        vulkanResourceDescriptorInfos = SystemPushArray<ElemGraphicsResourceDescriptorInfo>(VulkanGraphicsMemoryArena, VULKAN_MAX_RESOURCES, AllocationState_Reserved);
        vulkanResourceDescriptorImageViews = SystemPushArray<VkImageView>(VulkanGraphicsMemoryArena, VULKAN_MAX_RESOURCES, AllocationState_Reserved);
        vulkanSamplerInfos = SystemPushArray<VulkanGraphicsSamplerInfo>(VulkanGraphicsMemoryArena, VULKAN_MAX_SAMPLERS, AllocationState_Reserved);

        vulkanRaytracingInstanceMemoryArena = SystemAllocateMemoryArena(VULKAN_RAYTRACING_INSTANCE_MEMORY_ARENA);
        vulkanReadBackMemoryArena = SystemAllocateMemoryArena(VULKAN_READBACK_MEMORY_ARENA);
    }
}

VulkanGraphicsHeapData* GetVulkanGraphicsHeapData(ElemGraphicsHeap graphicsHeap)
{
    return SystemGetDataPoolItem(vulkanGraphicsHeapPool, graphicsHeap);
}

VulkanGraphicsResourceData* GetVulkanGraphicsResourceData(ElemGraphicsResource texture)
{
    return SystemGetDataPoolItem(vulkanGraphicsResourcePool, texture);
}

VulkanGraphicsResourceDataFull* GetVulkanGraphicsResourceDataFull(ElemGraphicsResource texture)
{
    return SystemGetDataPoolItemFull(vulkanGraphicsResourcePool, texture);
}

VkFormat ConvertToVulkanTextureFormat(ElemGraphicsFormat format)
{
    switch (format) 
    {
        case ElemGraphicsFormat_B8G8R8A8_SRGB:
            return VK_FORMAT_B8G8R8A8_SRGB;

        case ElemGraphicsFormat_B8G8R8A8:
            return VK_FORMAT_B8G8R8A8_UNORM;

        case ElemGraphicsFormat_R16G16B16A16_FLOAT:
            return VK_FORMAT_R16G16B16A16_SFLOAT;

        case ElemGraphicsFormat_R32G32B32A32_FLOAT:
            return VK_FORMAT_R32G32B32A32_SFLOAT;

        case ElemGraphicsFormat_R32G32B32_FLOAT:
            return VK_FORMAT_R32G32B32_SFLOAT;
        
        case ElemGraphicsFormat_D32_FLOAT:
            return VK_FORMAT_D32_SFLOAT;
        
        case ElemGraphicsFormat_R32_UINT:
            return VK_FORMAT_R32_UINT;

        case ElemGraphicsFormat_BC7:
            return VK_FORMAT_BC7_UNORM_BLOCK;

        case ElemGraphicsFormat_BC7_SRGB:
            return VK_FORMAT_BC7_SRGB_BLOCK;

        case ElemGraphicsFormat_Raw:
            return VK_FORMAT_UNDEFINED;
    }
}

VkImageUsageFlags ConvertToVulkanImageUsageFlags(ElemGraphicsResourceUsage usage)
{
    VkImageUsageFlags result = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    if (usage & ElemGraphicsResourceUsage_Write)
    {
        result |= VK_IMAGE_USAGE_STORAGE_BIT;
    }
    
    if (usage & ElemGraphicsResourceUsage_RenderTarget)
    {
        result |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    }

    if (usage & ElemGraphicsResourceUsage_DepthStencil)
    {
        result |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    }
    
    return result;
}

VkFilter ConvertToVulkanFilter(ElemGraphicsSamplerFilter filter)
{
    switch (filter)
    {
        case ElemGraphicsSamplerFilter_Nearest:
            return VK_FILTER_NEAREST;

        case ElemGraphicsSamplerFilter_Linear:
            return VK_FILTER_LINEAR;
    }
}

VkSamplerMipmapMode ConvertToVulkanSamplerMipMapMode(ElemGraphicsSamplerFilter filter)
{
    switch (filter)
    {
        case ElemGraphicsSamplerFilter_Nearest:
            return VK_SAMPLER_MIPMAP_MODE_NEAREST;

        case ElemGraphicsSamplerFilter_Linear:
            return VK_SAMPLER_MIPMAP_MODE_LINEAR;
    }
}

VkSamplerAddressMode ConvertToVulkanSamplerAddressMode(ElemGraphicsSamplerAddressMode addressMode)
{
    switch (addressMode)
    {
        case ElemGraphicsSamplerAddressMode_Repeat:
            return VK_SAMPLER_ADDRESS_MODE_REPEAT;

        case ElemGraphicsSamplerAddressMode_RepeatMirror:
            return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;

        case ElemGraphicsSamplerAddressMode_ClampToEdge:
            return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

        case ElemGraphicsSamplerAddressMode_ClampToEdgeMirror:
            return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;

        case ElemGraphicsSamplerAddressMode_ClampToBorderColor:
            return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    }
}

VkBuildAccelerationStructureFlagsKHR ConvertToVulkanRaytracingBuildFlags(ElemRaytracingBuildFlags buildFlags)
{
    VkBuildAccelerationStructureFlagsKHR result = 0u;

    if (buildFlags & ElemRaytracingBuildFlags_AllowUpdate)
    {
        result |= VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR ;
    }

    if (buildFlags & ElemRaytracingBuildFlags_AllowCompaction)
    {
        result |= VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR;
    }
    
    if (buildFlags & ElemRaytracingBuildFlags_PreferFastTrace)
    {
        result |= VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    }

    if (buildFlags & ElemRaytracingBuildFlags_PreferFastBuild)
    {
        result |= VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR;
    }

    if (buildFlags & ElemRaytracingBuildFlags_MinimizeMemory)
    {
        result |= VK_BUILD_ACCELERATION_STRUCTURE_LOW_MEMORY_BIT_KHR;
    }

    return result;
}

VkGeometryInstanceFlagsKHR ConvertToVulkanRaytracingInstanceFlags(ElemRaytracingTlasInstanceFlags instanceFlags)
{
    VkGeometryInstanceFlagsKHR result = 0u;
 
    if (instanceFlags & ElemRaytracingTlasInstanceFlags_DisableTriangleCulling)
    {
        result |= VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
    }

    if (instanceFlags & ElemRaytracingTlasInstanceFlags_FlipTriangleFaces)
    {
        result |= VK_GEOMETRY_INSTANCE_TRIANGLE_FLIP_FACING_BIT_KHR;
    }

    if (instanceFlags & ElemRaytracingTlasInstanceFlags_NonOpaque)
    {
        result |= VK_GEOMETRY_INSTANCE_FORCE_NO_OPAQUE_BIT_KHR;
    }

    return result;
}

VkAccelerationStructureBuildGeometryInfoKHR BuildVulkanBlasGeometryInfo(MemoryArena memoryArena, const ElemRaytracingBlasParameters* parameters)
{
    SystemAssert(parameters);
    auto description = SystemPushStruct<VkAccelerationStructureGeometryKHR>(memoryArena);

    *description =
    {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
        .geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR,
        .geometry =
        {
            .triangles =
            {
                .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR,
                .vertexFormat = ConvertToVulkanTextureFormat(parameters->VertexFormat),
                .vertexStride = parameters->VertexSizeInBytes,
                .maxVertex = parameters->VertexCount - 1,
                .indexType = VK_INDEX_TYPE_UINT32 // TODO: To change
            }
        },
        .flags = VK_GEOMETRY_OPAQUE_BIT_KHR
    };
    
    if (parameters->VertexBuffer != ELEM_HANDLE_NULL)
    {
        auto vertexBufferResourceData = GetVulkanGraphicsResourceData(parameters->VertexBuffer);
        SystemAssert(vertexBufferResourceData);

        auto vertexBufferResourceDataFull = GetVulkanGraphicsResourceDataFull(parameters->VertexBuffer);
        SystemAssert(vertexBufferResourceDataFull);

        auto graphicsDeviceData = GetVulkanGraphicsDeviceData(vertexBufferResourceDataFull->GraphicsDevice);
        SystemAssert(graphicsDeviceData);

        VkBufferDeviceAddressInfo deviceAddressInfo = { VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO };
        deviceAddressInfo.buffer = vertexBufferResourceData->BufferDeviceObject;

        VkDeviceAddress address = vkGetBufferDeviceAddress(graphicsDeviceData->Device, &deviceAddressInfo);
        SystemAssert(address);

        description->geometry.triangles.vertexData.deviceAddress = address + parameters->VertexBufferOffset;
    }

    if (parameters->IndexBuffer != ELEM_HANDLE_NULL)
    {
        auto indexBufferResourceData = GetVulkanGraphicsResourceData(parameters->IndexBuffer);
        SystemAssert(indexBufferResourceData);

        auto indexBufferResourceDataFull = GetVulkanGraphicsResourceDataFull(parameters->IndexBuffer);
        SystemAssert(indexBufferResourceDataFull);

        auto graphicsDeviceData = GetVulkanGraphicsDeviceData(indexBufferResourceDataFull->GraphicsDevice);
        SystemAssert(graphicsDeviceData);

        VkBufferDeviceAddressInfo deviceAddressInfo = { VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO };
        deviceAddressInfo.buffer = indexBufferResourceData->BufferDeviceObject;

        VkDeviceAddress address = vkGetBufferDeviceAddress(graphicsDeviceData->Device, &deviceAddressInfo);
        SystemAssert(address);

        description->geometry.triangles.indexData.deviceAddress = address + parameters->IndexBufferOffset;
    }

    return
    {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
        .type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
        .flags = ConvertToVulkanRaytracingBuildFlags(parameters->BuildFlags), 
        .mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
        .geometryCount = 1,
        .pGeometries = description
    };
}

VkAccelerationStructureBuildGeometryInfoKHR BuildVulkanTlasGeometryInfo(MemoryArena memoryArena, const ElemRaytracingTlasParameters* parameters)
{
    SystemAssert(parameters);
    auto description = SystemPushStruct<VkAccelerationStructureGeometryKHR>(memoryArena);

    *description =
    {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
        .geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR,
        .geometry =
        {
            .instances = 
            {
                .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR
            }
        }
    };
    
    if (parameters->InstanceBuffer)
    {
        auto instanceBufferResourceData = GetVulkanGraphicsResourceData(parameters->InstanceBuffer);
        SystemAssert(instanceBufferResourceData);

        auto instanceBufferResourceDataFull = GetVulkanGraphicsResourceDataFull(parameters->InstanceBuffer);
        SystemAssert(instanceBufferResourceDataFull);

        auto graphicsDeviceData = GetVulkanGraphicsDeviceData(instanceBufferResourceDataFull->GraphicsDevice);
        SystemAssert(graphicsDeviceData);

        VkBufferDeviceAddressInfo deviceAddressInfo = { VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO };
        deviceAddressInfo.buffer = instanceBufferResourceData->BufferDeviceObject;

        VkDeviceAddress address = vkGetBufferDeviceAddress(graphicsDeviceData->Device, &deviceAddressInfo);
        SystemAssert(address);

        description->geometry.instances.data.deviceAddress = address + parameters->InstanceBufferOffset;
    }

    return
    {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
        .type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
        .flags = ConvertToVulkanRaytracingBuildFlags(parameters->BuildFlags), 
        .mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
        .geometryCount = 1,
        .pGeometries = description
    };
}

ElemGraphicsResource CreateVulkanTextureFromResource(ElemGraphicsDevice graphicsDevice, VkImage resource, const ElemGraphicsResourceInfo* resourceInfo, bool isPresentTexture)
{
    InitVulkanResourceMemory();

    auto graphicsDeviceData = GetVulkanGraphicsDeviceData(graphicsDevice);
    SystemAssert(graphicsDeviceData);

    auto graphicsDeviceDataFull = GetVulkanGraphicsDeviceDataFull(graphicsDevice);
    SystemAssert(graphicsDeviceDataFull);

    auto vulkanTextureFormat = ConvertToVulkanTextureFormat(resourceInfo->Format);
    
    VkImageView renderTargetImageView = {};
    VkImageView depthStencilImageView = {};

    if (resourceInfo->Usage & ElemGraphicsResourceUsage_RenderTarget)
    {
        VkImageViewCreateInfo createInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        createInfo.image = resource;
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = vulkanTextureFormat;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
        createInfo.subresourceRange.layerCount = 1;

        AssertIfFailed(vkCreateImageView(graphicsDeviceData->Device, &createInfo, 0, &renderTargetImageView));
    }
    else if (resourceInfo->Usage & ElemGraphicsResourceUsage_DepthStencil)
    {
        VkImageViewCreateInfo createInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        createInfo.image = resource;
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = vulkanTextureFormat;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT; // TODO: Maybe we need to combine that with flag Stencil depending on the format
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
        createInfo.subresourceRange.layerCount = 1;

        AssertIfFailed(vkCreateImageView(graphicsDeviceData->Device, &createInfo, 0, &depthStencilImageView));
    }

    auto handle = SystemAddDataPoolItem(vulkanGraphicsResourcePool, {
        .TextureDeviceObject = resource,
        .Type = ElemGraphicsResourceType_Texture2D,
        .RenderTargetImageView = renderTargetImageView,
        .DepthStencilImageView = depthStencilImageView,
        .Format = vulkanTextureFormat,
        .InternalFormat = resourceInfo->Format,
        .IsPresentTexture = isPresentTexture,
        .Width = resourceInfo->Width,
        .Height = resourceInfo->Height,
        .MipLevels = resourceInfo->MipLevels,
        .Usage = resourceInfo->Usage
    }); 

    SystemAddDataPoolItemFull(vulkanGraphicsResourcePool, handle, {
        .GraphicsDevice = graphicsDevice
    });

    return handle;
}

VkImage CreateVulkanTexture(ElemGraphicsDevice graphicsDevice, const ElemGraphicsResourceInfo* resourceInfo)
{
    SystemAssert(graphicsDevice != ELEM_HANDLE_NULL);
    SystemAssert(resourceInfo);

    auto graphicsDeviceData = GetVulkanGraphicsDeviceData(graphicsDevice);
    SystemAssert(graphicsDeviceData);

    VkImageCreateInfo createInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    createInfo.imageType = VK_IMAGE_TYPE_2D;
    createInfo.format = ConvertToVulkanTextureFormat(resourceInfo->Format);
    createInfo.extent.width = resourceInfo->Width;
    createInfo.extent.height = resourceInfo->Height;
    createInfo.extent.depth = 1;
    createInfo.mipLevels = resourceInfo->MipLevels;
    createInfo.arrayLayers = 1;
    createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    createInfo.usage = ConvertToVulkanImageUsageFlags(resourceInfo->Usage);

	VkImage image = nullptr;
    AssertIfFailed(vkCreateImage(graphicsDeviceData->Device, &createInfo, nullptr, &image));

    if (VulkanDebugLayerEnabled && resourceInfo->DebugName)
    {
        VkDebugUtilsObjectNameInfoEXT nameInfo = { VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT };
        nameInfo.objectType = VK_OBJECT_TYPE_IMAGE;
        nameInfo.objectHandle = (uint64_t)image;
        nameInfo.pObjectName = resourceInfo->DebugName;

        AssertIfFailed(vkSetDebugUtilsObjectNameEXT(graphicsDeviceData->Device, &nameInfo)); 
    }

    return image;
}

VkBuffer CreateVulkanBuffer(ElemGraphicsDevice graphicsDevice, const ElemGraphicsResourceInfo* resourceInfo, bool isAccelerationStructure)
{
    SystemAssert(graphicsDevice != ELEM_HANDLE_NULL);
    SystemAssert(resourceInfo);

    auto graphicsDeviceData = GetVulkanGraphicsDeviceData(graphicsDevice);
    SystemAssert(graphicsDeviceData);

    VkBufferCreateInfo createInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    createInfo.size = resourceInfo->Width;
    // TODO: Review the accel struct read usage
    createInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;

    if (isAccelerationStructure)
    {
        createInfo.usage |= VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR;
    }

    VkBuffer buffer;
    AssertIfFailed(vkCreateBuffer(graphicsDeviceData->Device, &createInfo, nullptr, &buffer));

    if (VulkanDebugLayerEnabled && resourceInfo->DebugName)
    {
        VkDebugUtilsObjectNameInfoEXT nameInfo = { VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT };
        nameInfo.objectType = VK_OBJECT_TYPE_BUFFER;
        nameInfo.objectHandle = (uint64_t)buffer;
        nameInfo.pObjectName = resourceInfo->DebugName;

        AssertIfFailed(vkSetDebugUtilsObjectNameEXT(graphicsDeviceData->Device, &nameInfo)); 
    }

    return buffer;
}

ElemGraphicsHeap VulkanCreateGraphicsHeap(ElemGraphicsDevice graphicsDevice, uint64_t sizeInBytes, const ElemGraphicsHeapOptions* options)
{
    auto stackMemoryArena = SystemGetStackMemoryArena();
    InitVulkanResourceMemory();
    
    SystemAssert(graphicsDevice != ELEM_HANDLE_NULL);

    auto graphicsDeviceData = GetVulkanGraphicsDeviceData(graphicsDevice);
    SystemAssert(graphicsDeviceData);

    auto graphicsDeviceDataFull = GetVulkanGraphicsDeviceDataFull(graphicsDevice);
    SystemAssert(graphicsDeviceDataFull);

    VkMemoryAllocateFlagsInfo flagInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO };
    flagInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;

    VkMemoryAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    allocateInfo.allocationSize = sizeInBytes;
    allocateInfo.memoryTypeIndex = graphicsDeviceDataFull->GpuMemoryTypeIndex;
    allocateInfo.pNext = &flagInfo;

    auto heapType = ElemGraphicsHeapType_Gpu;

    if (options)
    {
        heapType = options->HeapType;

        if (options->HeapType == ElemGraphicsHeapType_GpuUpload)
        {
            allocateInfo.memoryTypeIndex = graphicsDeviceDataFull->GpuUploadMemoryTypeIndex;
        }
        else if (options->HeapType == ElemGraphicsHeapType_Readback)
        {
            allocateInfo.memoryTypeIndex = graphicsDeviceDataFull->ReadBackMemoryTypeIndex;
        }
    }

    VkDeviceMemory deviceMemory;
    AssertIfFailed(vkAllocateMemory(graphicsDeviceData->Device, &allocateInfo, nullptr, &deviceMemory));
    
    if (VulkanDebugLayerEnabled && options && options->DebugName)
    {
        VkDebugUtilsObjectNameInfoEXT nameInfo = { VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT };
        nameInfo.objectType = VK_OBJECT_TYPE_DEVICE_MEMORY;
        nameInfo.objectHandle = (uint64_t)deviceMemory;
        nameInfo.pObjectName = options->DebugName;

        AssertIfFailed(vkSetDebugUtilsObjectNameEXT(graphicsDeviceData->Device, &nameInfo)); 
    }

    auto handle = SystemAddDataPoolItem(vulkanGraphicsHeapPool, {
        .DeviceObject = deviceMemory,
        .HeapType = heapType,
        .SizeInBytes = sizeInBytes,
        .GraphicsDevice = graphicsDevice,
    }); 

    return handle;
}

void VulkanFreeGraphicsHeap(ElemGraphicsHeap graphicsHeap)
{
    SystemAssert(graphicsHeap != ELEM_HANDLE_NULL);

    auto graphicsHeapData = GetVulkanGraphicsHeapData(graphicsHeap);
    SystemAssert(graphicsHeapData);

    auto graphicsDeviceData = GetVulkanGraphicsDeviceData(graphicsHeapData->GraphicsDevice);
    SystemAssert(graphicsDeviceData);

    vkFreeMemory(graphicsDeviceData->Device, graphicsHeapData->DeviceObject, nullptr);
    SystemRemoveDataPoolItem(vulkanGraphicsHeapPool, graphicsHeap);
}

ElemGraphicsResourceInfo VulkanCreateGraphicsBufferResourceInfo(ElemGraphicsDevice graphicsDevice, uint32_t sizeInBytes, ElemGraphicsResourceUsage usage, const ElemGraphicsResourceInfoOptions* options)
{
    SystemAssert(graphicsDevice != ELEM_HANDLE_NULL);

    auto graphicsDeviceData = GetVulkanGraphicsDeviceData(graphicsDevice);
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

    auto buffer = CreateVulkanBuffer(graphicsDevice, &resourceInfo, usage & ElemGraphicsResourceUsage_RaytracingAccelerationStructure);

    VkMemoryRequirements memoryRequirements = {};
    vkGetBufferMemoryRequirements(graphicsDeviceData->Device, buffer, &memoryRequirements);

    vkDestroyBuffer(graphicsDeviceData->Device, buffer, nullptr);

    resourceInfo.Alignment = memoryRequirements.alignment;
    resourceInfo.SizeInBytes = memoryRequirements.size;

    return resourceInfo;
}

ElemGraphicsResourceInfo VulkanCreateTexture2DResourceInfo(ElemGraphicsDevice graphicsDevice, uint32_t width, uint32_t height, uint32_t mipLevels, ElemGraphicsFormat format, ElemGraphicsResourceUsage usage, const ElemGraphicsResourceInfoOptions* options)
{
    SystemAssert(graphicsDevice != ELEM_HANDLE_NULL);

    auto graphicsDeviceData = GetVulkanGraphicsDeviceData(graphicsDevice);
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

    auto texture = CreateVulkanTexture(graphicsDevice, &resourceInfo);

    VkMemoryRequirements memoryRequirements = {};
    vkGetImageMemoryRequirements(graphicsDeviceData->Device, texture, &memoryRequirements);

    vkDestroyImage(graphicsDeviceData->Device, texture, nullptr);

    resourceInfo.Alignment = memoryRequirements.alignment;
    resourceInfo.SizeInBytes = memoryRequirements.size;

    return resourceInfo;
}

ElemGraphicsResource VulkanCreateGraphicsResource(ElemGraphicsHeap graphicsHeap, uint64_t graphicsHeapOffset, const ElemGraphicsResourceInfo* resourceInfo)
{
    SystemAssert(graphicsHeap != ELEM_HANDLE_NULL);
    SystemAssert(resourceInfo);

    auto graphicsHeapData = GetVulkanGraphicsHeapData(graphicsHeap);
    SystemAssert(graphicsHeapData);

    auto graphicsDeviceData = GetVulkanGraphicsDeviceData(graphicsHeapData->GraphicsDevice);
    SystemAssert(graphicsDeviceData);

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

        auto texture = CreateVulkanTexture(graphicsHeapData->GraphicsDevice, resourceInfo);
        AssertIfFailed(vkBindImageMemory(graphicsDeviceData->Device, texture, graphicsHeapData->DeviceObject, graphicsHeapOffset));

        return CreateVulkanTextureFromResource(graphicsHeapData->GraphicsDevice, texture, resourceInfo, false);
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
        
        if (resourceInfo->Usage & ElemGraphicsResourceUsage_DepthStencil)
        {
            SystemLogErrorMessage(ElemLogMessageCategory_Graphics, "GraphicsBuffer usage should not be equals to DepthStencil.");
            return ELEM_HANDLE_NULL;
        }

        if (resourceInfo->Usage & ElemGraphicsResourceUsage_RaytracingAccelerationStructure && graphicsHeapData->HeapType != ElemGraphicsHeapType_Gpu)
        {
            SystemLogErrorMessage(ElemLogMessageCategory_Graphics, "GraphicsBuffer with usage RaytracingAccelerationStructure should be allocated on a Gpu Heap.");
            return ELEM_HANDLE_NULL;
        }

        auto buffer = CreateVulkanBuffer(graphicsHeapData->GraphicsDevice, resourceInfo, resourceInfo->Usage & ElemGraphicsResourceUsage_RaytracingAccelerationStructure);

        AssertIfFailed(vkBindBufferMemory(graphicsDeviceData->Device, buffer, graphicsHeapData->DeviceObject, graphicsHeapOffset));

        auto handle = SystemAddDataPoolItem(vulkanGraphicsResourcePool, {
            .BufferDeviceObject = buffer,
            .Type = ElemGraphicsResourceType_Buffer,
            .Width = resourceInfo->Width,
            .Usage = resourceInfo->Usage
        }); 

        SystemAddDataPoolItemFull(vulkanGraphicsResourcePool, handle, {
            .GraphicsDevice = graphicsHeapData->GraphicsDevice,
            .GraphicsHeap = graphicsHeap,
            .GraphicsHeapOffset = graphicsHeapOffset
        });

        return handle;
    }
}

void VulkanFreeGraphicsResource(ElemGraphicsResource resource, const ElemFreeGraphicsResourceOptions* options)
{
    SystemAssert(resource != ELEM_HANDLE_NULL);

    if (options && options->FencesToWait.Length > 0)
    {
        EnqueueResourceDeleteEntry(VulkanGraphicsMemoryArena, resource, ResourceDeleteType_Resource, options->FencesToWait);
        return;
    }

    auto resourceData = GetVulkanGraphicsResourceData(resource);

    if (resourceData)
    {
        auto resourceDataFull = GetVulkanGraphicsResourceDataFull(resource);
        SystemAssert(resourceDataFull);

        auto graphicsDeviceData = GetVulkanGraphicsDeviceData(resourceDataFull->GraphicsDevice);
        SystemAssert(graphicsDeviceData);

        if (resourceData->BufferDeviceObject)
        {
            vkDestroyBuffer(graphicsDeviceData->Device, resourceData->BufferDeviceObject, nullptr);
        }

        if (resourceData->AccelerationStructureDeviceObject)
        {
            vkDestroyAccelerationStructureKHR(graphicsDeviceData->Device, resourceData->AccelerationStructureDeviceObject, nullptr);
        }
    
        if (resourceData->Usage & ElemGraphicsResourceUsage_RenderTarget)
        {
            vkDestroyImageView(graphicsDeviceData->Device, resourceData->RenderTargetImageView, nullptr);
        }

        if (resourceData->Usage & ElemGraphicsResourceUsage_DepthStencil)
        {
            vkDestroyImageView(graphicsDeviceData->Device, resourceData->DepthStencilImageView, nullptr);
        }

        if (resourceData->TextureDeviceObject && !resourceData->IsPresentTexture)
        {
            vkDestroyImage(graphicsDeviceData->Device, resourceData->TextureDeviceObject, nullptr);
        }

        SystemRemoveDataPoolItem(vulkanGraphicsResourcePool, resource);
    }
}

ElemGraphicsResourceInfo VulkanGetGraphicsResourceInfo(ElemGraphicsResource resource)
{
    SystemAssert(resource != ELEM_HANDLE_NULL);

    auto resourceData = GetVulkanGraphicsResourceData(resource);

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
        .Format = resourceData->InternalFormat,
        .Alignment = 0,
        .SizeInBytes = 0,
        .Usage = resourceData->Usage
    };
}

void VulkanUploadGraphicsBufferData(ElemGraphicsResource buffer, uint32_t offset, ElemDataSpan data)
{
    SystemAssert(buffer != ELEM_HANDLE_NULL);

    auto resourceData = GetVulkanGraphicsResourceData(buffer);
    SystemAssert(resourceData);

    if (resourceData->Type != ElemGraphicsResourceType_Buffer)
    {
        SystemLogErrorMessage(ElemLogMessageCategory_Graphics, "ElemUploadGraphicsBufferData only works with graphics buffers.");
        return;
    }

    auto resourceDataFull = GetVulkanGraphicsResourceDataFull(buffer);
    SystemAssert(resourceDataFull);

    auto graphicsHeapData = GetVulkanGraphicsHeapData(resourceDataFull->GraphicsHeap);
    SystemAssert(graphicsHeapData);

    if (graphicsHeapData->HeapType != ElemGraphicsHeapType_GpuUpload)
    {
        SystemLogErrorMessage(ElemLogMessageCategory_Graphics, "ElemUploadGraphicsBufferData only works with graphics buffers allocated in a GpuUpload heap.");
        return;
    }
	
    auto graphicsDeviceData = GetVulkanGraphicsDeviceData(resourceDataFull->GraphicsDevice);
    SystemAssert(graphicsDeviceData);

    uint8_t* cpuPointer = nullptr;
    AssertIfFailed(vkMapMemory(graphicsDeviceData->Device, graphicsHeapData->DeviceObject, resourceDataFull->GraphicsHeapOffset + offset, data.Length, 0, (void**)&cpuPointer));

    memcpy(cpuPointer, data.Items, data.Length);

    vkUnmapMemory(graphicsDeviceData->Device, graphicsHeapData->DeviceObject);
}

ElemDataSpan VulkanDownloadGraphicsBufferData(ElemGraphicsResource buffer, const ElemDownloadGraphicsBufferDataOptions* options)
{
    SystemAssert(buffer != ELEM_HANDLE_NULL);

    auto resourceData = GetVulkanGraphicsResourceData(buffer);
    SystemAssert(resourceData);

    if (resourceData->Type != ElemGraphicsResourceType_Buffer)
    {
        SystemLogErrorMessage(ElemLogMessageCategory_Graphics, "ElemDownloadGraphicsBufferData only works with graphics buffers.");
        return {};
    }

    auto resourceDataFull = GetVulkanGraphicsResourceDataFull(buffer);
    SystemAssert(resourceDataFull);

    auto graphicsHeapData = GetVulkanGraphicsHeapData(resourceDataFull->GraphicsHeap);
    SystemAssert(graphicsHeapData);

    if (graphicsHeapData->HeapType != ElemGraphicsHeapType_GpuUpload && graphicsHeapData->HeapType != ElemGraphicsHeapType_Readback)
    {
        SystemLogErrorMessage(ElemLogMessageCategory_Graphics, "ElemDownloadGraphicsBufferData only works with graphics buffers allocated in a Readback heap or GpuUpload heap. (%d)", graphicsHeapData->HeapType);
        return {};
    }

    if (graphicsHeapData->HeapType != ElemGraphicsHeapType_Readback)
    {
        SystemLogWarningMessage(ElemLogMessageCategory_Graphics, "ElemDownloadGraphicsBufferData works faster with graphics buffers allocated in a Readback heap.");
    }

    auto graphicsDeviceData = GetVulkanGraphicsDeviceData(resourceDataFull->GraphicsDevice);
    SystemAssert(graphicsDeviceData);

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

    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto downloadedData = SystemPushArray<uint8_t>(vulkanReadBackMemoryArena, sizeInBytes);

    uint8_t* cpuPointer = nullptr;
    AssertIfFailed(vkMapMemory(graphicsDeviceData->Device, graphicsHeapData->DeviceObject, resourceDataFull->GraphicsHeapOffset + offset, sizeInBytes, 0, (void**)&cpuPointer));

    memcpy(downloadedData.Pointer, cpuPointer, sizeInBytes);

    vkUnmapMemory(graphicsDeviceData->Device, graphicsHeapData->DeviceObject);

	return { .Items = downloadedData.Pointer, .Length = (uint32_t)downloadedData.Length };
}

VulkanUploadBuffer CreateVulkanUploadBuffer(ElemGraphicsDevice graphicsDevice, uint64_t sizeInBytes)
{
    SystemLogDebugMessage(ElemLogMessageCategory_Graphics, "Create Vulkan UploadBuffer with : %d", sizeInBytes);
    SystemAssert(graphicsDevice);

    auto graphicsDeviceData = GetVulkanGraphicsDeviceData(graphicsDevice);
    SystemAssert(graphicsDeviceData);

    auto graphicsDeviceDataFull = GetVulkanGraphicsDeviceDataFull(graphicsDevice);
    SystemAssert(graphicsDeviceDataFull);

    VkBufferCreateInfo createInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    createInfo.size = sizeInBytes;
    createInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

    VkBuffer buffer;
    AssertIfFailed(vkCreateBuffer(graphicsDeviceData->Device, &createInfo, nullptr, &buffer));

    if (VulkanDebugLayerEnabled)
    {
        VkDebugUtilsObjectNameInfoEXT nameInfo = { VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT };
        nameInfo.objectType = VK_OBJECT_TYPE_BUFFER;
        nameInfo.objectHandle = (uint64_t)buffer;
        nameInfo.pObjectName = "ElementalUploadBuffer";

        AssertIfFailed(vkSetDebugUtilsObjectNameEXT(graphicsDeviceData->Device, &nameInfo)); 
    }

    VkMemoryAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    allocateInfo.allocationSize = sizeInBytes;
    allocateInfo.memoryTypeIndex = graphicsDeviceDataFull->UploadMemoryTypeIndex;

    VkDeviceMemory deviceMemory;
    AssertIfFailed(vkAllocateMemory(graphicsDeviceData->Device, &allocateInfo, nullptr, &deviceMemory));
    AssertIfFailed(vkBindBufferMemory(graphicsDeviceData->Device, buffer, deviceMemory, 0));

    return
    {
        .Buffer = buffer,
        .DeviceMemory = deviceMemory
    };
}

UploadBufferMemory<VulkanUploadBuffer> GetVulkanUploadBuffer(ElemGraphicsDevice graphicsDevice, uint64_t alignment, uint64_t sizeInBytes)
{
    auto graphicsDeviceData = GetVulkanGraphicsDeviceData(graphicsDevice);
    SystemAssert(graphicsDeviceData);

    auto graphicsIdUnpacked = UnpackSystemDataPoolHandle(graphicsDevice);
    auto uploadBufferPool = &threadVulkanUploadBufferPools[graphicsIdUnpacked.Index];

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
            VulkanWaitForFenceOnCpu(uploadBuffer.PoolItem->Fence);
        }
        
        if (uploadBuffer.PoolItem->Buffer.Buffer != nullptr)
        {
            SystemLogDebugMessage(ElemLogMessageCategory_Graphics, "Need to delete buffer");

            vkDestroyBuffer(graphicsDeviceData->Device, uploadBuffer.PoolItem->Buffer.Buffer, nullptr);
            vkFreeMemory(graphicsDeviceData->Device, uploadBuffer.PoolItem->Buffer.DeviceMemory, nullptr);

            uploadBuffer.PoolItem->Buffer = {};
        }

        uploadBuffer.PoolItem->Buffer = CreateVulkanUploadBuffer(graphicsDevice, uploadBuffer.PoolItem->SizeInBytes);
        uploadBuffer.PoolItem->IsResetNeeded = false;
    }

    return uploadBuffer;
}

void CreateVulkanCopyTextureBarrier(VkCommandBuffer commandBuffer, VkImage image, uint32_t mipLevel, bool beforeCopy)
{
    VkImageMemoryBarrier2 barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = mipLevel;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

    if (beforeCopy)
    {
        barrier.srcStageMask = VK_PIPELINE_STAGE_2_NONE;
        barrier.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
        barrier.srcAccessMask = VK_ACCESS_2_NONE;
        barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    }
    else
    {
        barrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
        barrier.dstStageMask = VK_PIPELINE_STAGE_2_MESH_SHADER_BIT_EXT | 
                               VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT | 
                               VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
        barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }
    
    VkDependencyInfo dependencyInfo = { VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
    dependencyInfo.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    dependencyInfo.imageMemoryBarrierCount = 1;
    dependencyInfo.pImageMemoryBarriers = &barrier;

    vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);
}

void VulkanCopyDataToGraphicsResource(ElemCommandList commandList, const ElemCopyDataToGraphicsResourceParameters* parameters)
{
    // TODO: Implement optimizations on the copy queue. On windows, use DirectStorage
    auto stackMemoryArena = SystemGetStackMemoryArena();

    SystemAssert(commandList != ELEM_HANDLE_NULL);

    SystemAssert(parameters);
    SystemAssert(parameters->Resource != ELEM_HANDLE_NULL);

    auto commandListData = GetVulkanCommandListData(commandList);
    SystemAssert(commandListData);

    auto graphicsDeviceData = GetVulkanGraphicsDeviceData(commandListData->GraphicsDevice);
    SystemAssert(graphicsDeviceData);

    auto resourceData = GetVulkanGraphicsResourceData(parameters->Resource);
    SystemAssert(resourceData);

    auto resourceDataFull = GetVulkanGraphicsResourceDataFull(parameters->Resource);
    SystemAssert(resourceDataFull);

    ReadOnlySpan<uint8_t> sourceData;

    // TODO: Implement file source
    if (parameters->SourceType == ElemCopyDataSourceType_Memory)
    {
        sourceData = ReadOnlySpan<uint8_t>(parameters->SourceMemoryData.Items, parameters->SourceMemoryData.Length);
    }
    
    auto uploadBufferAlignment = 4u;
    auto uploadBufferSizeInBytes = sourceData.Length;

    auto uploadBuffer = GetVulkanUploadBuffer(commandListData->GraphicsDevice, uploadBufferAlignment, uploadBufferSizeInBytes);
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
    
    uint8_t* cpuPointer = nullptr;
    AssertIfFailed(vkMapMemory(graphicsDeviceData->Device, uploadBuffer.PoolItem->Buffer.DeviceMemory, uploadBuffer.Offset, sourceData.Length, 0, (void**)&cpuPointer));

    memcpy(cpuPointer, sourceData.Pointer, sourceData.Length);

    if (resourceData->Type == ElemGraphicsResourceType_Buffer)
    {
        VkBufferCopy copyRegion
        {
            .srcOffset = uploadBuffer.Offset,
            .dstOffset = parameters->BufferOffset,
            .size = sourceData.Length
        };
     
        vkCmdCopyBuffer(commandListData->DeviceObject, uploadBuffer.PoolItem->Buffer.Buffer, resourceData->BufferDeviceObject, 1, &copyRegion);
    }
    else if (resourceData->Type == ElemGraphicsResourceType_Texture2D)
    {
        auto mipLevel = parameters->TextureMipLevel;
        auto mipWidth  = SystemMax(1u, resourceData->Width  >> mipLevel);
        auto mipHeight = SystemMax(1u, resourceData->Height >> mipLevel);

        CreateVulkanCopyTextureBarrier(commandListData->DeviceObject, resourceData->TextureDeviceObject, mipLevel, true);

        VkBufferImageCopy region = 
        {
			.bufferOffset = uploadBuffer.Offset,
			.bufferRowLength = 0,
			.bufferImageHeight = 0,
			.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, mipLevel, 0, 1 },
			.imageOffset = { 0, 0, 0 },
			.imageExtent = { mipWidth, mipHeight, 1 },
		};
        
		vkCmdCopyBufferToImage(commandListData->DeviceObject, uploadBuffer.PoolItem->Buffer.Buffer, resourceData->TextureDeviceObject, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        CreateVulkanCopyTextureBarrier(commandListData->DeviceObject, resourceData->TextureDeviceObject, mipLevel, false);
    }

    vkUnmapMemory(graphicsDeviceData->Device, uploadBuffer.PoolItem->Buffer.DeviceMemory);
}

ElemGraphicsResourceDescriptor VulkanCreateGraphicsResourceDescriptor(ElemGraphicsResource resource, ElemGraphicsResourceDescriptorUsage usage, const ElemGraphicsResourceDescriptorOptions* options)
{
    SystemAssert(resource != ELEM_HANDLE_NULL);

    auto resourceData = GetVulkanGraphicsResourceData(resource);
    SystemAssert(resourceData);

    auto resourceUsage = resourceData->Usage;

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

    auto resourceDataFull = GetVulkanGraphicsResourceDataFull(resource);
    SystemAssert(resourceDataFull);

    auto graphicsDeviceData = GetVulkanGraphicsDeviceData(resourceDataFull->GraphicsDevice);
    SystemAssert(graphicsDeviceData);

    auto textureMipIndex = 0u;

    if (options)
    {
        textureMipIndex = options->TextureMipIndex;
    }

    auto descriptorHeap = graphicsDeviceData->ResourceDescriptorHeap;
    auto descriptorHandle = CreateVulkanDescriptorHandle(descriptorHeap);

    if ((descriptorHandle % 1000) == 0)
    {
        SystemCommitMemory(VulkanGraphicsMemoryArena, &vulkanResourceDescriptorInfos[descriptorHandle], 1000 *  sizeof(ElemGraphicsResourceDescriptorInfo));
        SystemCommitMemory(VulkanGraphicsMemoryArena, &vulkanResourceDescriptorImageViews[descriptorHandle], 1000 *  sizeof(VkImageView));
    }
    
    VkWriteDescriptorSet descriptor = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
    descriptor.dstSet = descriptorHeap.Storage->DescriptorSet->DescriptorSet;
    descriptor.dstBinding = 0;
    descriptor.dstArrayElement = descriptorHandle;
    descriptor.descriptorCount = 1;

    if (resourceData->Type == ElemGraphicsResourceType_Buffer)
    {
        VkDescriptorBufferInfo bufferInfo = {};
        bufferInfo.buffer = resourceData->BufferDeviceObject;
        bufferInfo.range = resourceData->Width;

        descriptor.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptor.pBufferInfo = &bufferInfo;

    }
    else if (resourceData->Type == ElemGraphicsResourceType_RaytracingAccelerationStructure)
    {
        VkWriteDescriptorSetAccelerationStructureKHR accelerationStructInfo = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR };
        accelerationStructInfo.accelerationStructureCount = 1;
        accelerationStructInfo.pAccelerationStructures = &resourceData->AccelerationStructureDeviceObject;

        descriptor.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
        descriptor.pNext = &accelerationStructInfo;
    }
    else
    {
        VkImageViewCreateInfo createInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        createInfo.image = resourceData->TextureDeviceObject;
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = resourceData->Format;
        createInfo.subresourceRange.aspectMask = (resourceData->Usage & ElemGraphicsResourceUsage_DepthStencil) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT; // TODO: Stencil?
        createInfo.subresourceRange.baseMipLevel = textureMipIndex;
        createInfo.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
        createInfo.subresourceRange.layerCount = 1;

        VkImageView imageView;
        AssertIfFailed(vkCreateImageView(graphicsDeviceData->Device, &createInfo, 0, &imageView));

        VkDescriptorImageInfo imageInfo = {};
        imageInfo.imageView = imageView;
        imageInfo.imageLayout = (usage == ElemGraphicsResourceDescriptorUsage_Read) ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_GENERAL;

        descriptor.descriptorType = (usage == ElemGraphicsResourceDescriptorUsage_Read) ? VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE : VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        descriptor.pImageInfo = &imageInfo;

        vulkanResourceDescriptorImageViews[descriptorHandle] = imageView;
    }

    vkUpdateDescriptorSets(graphicsDeviceData->Device, 1, &descriptor, 0, nullptr);

    if ((descriptorHandle % 1024) == 0)
    {
        SystemCommitMemory(VulkanGraphicsMemoryArena, &vulkanResourceDescriptorInfos[descriptorHandle], 1024 *  sizeof(ElemGraphicsResourceDescriptorInfo));
    }

    vulkanResourceDescriptorInfos[descriptorHandle].Resource = resource;
    vulkanResourceDescriptorInfos[descriptorHandle].Usage = usage;

    return descriptorHandle;
}

ElemGraphicsResourceDescriptorInfo VulkanGetGraphicsResourceDescriptorInfo(ElemGraphicsResourceDescriptor descriptor)
{
    if (descriptor == -1)
    {
        SystemLogErrorMessage(ElemLogMessageCategory_Graphics, "Resource Descriptor is invalid.");
        return {};
    }

    return vulkanResourceDescriptorInfos[descriptor];
}

void VulkanFreeGraphicsResourceDescriptor(ElemGraphicsResourceDescriptor descriptor, const ElemFreeGraphicsResourceDescriptorOptions* options)
{
    if (descriptor == -1)
    {
        SystemLogErrorMessage(ElemLogMessageCategory_Graphics, "Resource Descriptor is invalid.");
        return;
    }

    if (options && options->FencesToWait.Length > 0)
    {
        EnqueueResourceDeleteEntry(VulkanGraphicsMemoryArena, descriptor, ResourceDeleteType_Descriptor, options->FencesToWait);
        return;
    }

    if (vulkanResourceDescriptorInfos[descriptor].Resource != ELEM_HANDLE_NULL)
    {
        auto resourceData = GetVulkanGraphicsResourceData(vulkanResourceDescriptorInfos[descriptor].Resource);
        SystemAssert(resourceData);

        auto resourceDataFull = GetVulkanGraphicsResourceDataFull(vulkanResourceDescriptorInfos[descriptor].Resource);
        SystemAssert(resourceDataFull);

        if (resourceData->Type != ElemGraphicsResourceType_Buffer)
        {
            auto graphicsDeviceData = GetVulkanGraphicsDeviceData(resourceDataFull->GraphicsDevice);
            SystemAssert(graphicsDeviceData);

            vkDestroyImageView(graphicsDeviceData->Device, vulkanResourceDescriptorImageViews[descriptor], nullptr);
            vulkanResourceDescriptorImageViews[descriptor] = {};
        }
    }

    vulkanResourceDescriptorInfos[descriptor].Resource = ELEM_HANDLE_NULL;
}

void VulkanProcessGraphicsResourceDeleteQueue(ElemGraphicsDevice graphicsDevice)
{
    auto stackMemoryArena = SystemGetStackMemoryArena();

    ProcessResourceDeleteQueue();
    SystemClearMemoryArena(vulkanReadBackMemoryArena);

    auto graphicsDeviceData = GetVulkanGraphicsDeviceData(graphicsDevice);
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

                vkDestroyBuffer(graphicsDeviceData->Device, uploadBufferToDelete->Buffer.Buffer, nullptr);
                vkFreeMemory(graphicsDeviceData->Device, uploadBufferToDelete->Buffer.DeviceMemory, nullptr);

                uploadBufferToDelete->Buffer = {};

                uploadBufferToDelete->CurrentOffset = 0;
                uploadBufferToDelete->SizeInBytes = 0;
            }
        }
    }
}

ElemGraphicsSampler VulkanCreateGraphicsSampler(ElemGraphicsDevice graphicsDevice, const ElemGraphicsSamplerInfo* samplerInfo)
{
    InitVulkanResourceMemory();

    auto graphicsDeviceData = GetVulkanGraphicsDeviceData(graphicsDevice);
    SystemAssert(graphicsDeviceData);

    auto descriptorHeap = graphicsDeviceData->SamplerDescriptorHeap;
    auto descriptorHandle = CreateVulkanDescriptorHandle(descriptorHeap);

    auto localSamplerInfo = *samplerInfo;

    if (localSamplerInfo.MaxAnisotropy == 0)
    {
        localSamplerInfo.MaxAnisotropy = 1;
    }

    auto borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;

    if (localSamplerInfo.BorderColor.Red == 1.0f && localSamplerInfo.BorderColor.Green == 1.0f && localSamplerInfo.BorderColor.Blue == 1.0f && localSamplerInfo.BorderColor.Alpha == 1.0f)
    {
        borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    }

    VkSamplerCreateInfo samplerCreateInfo = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
    samplerCreateInfo.minFilter = ConvertToVulkanFilter(localSamplerInfo.MinFilter);
    samplerCreateInfo.magFilter = ConvertToVulkanFilter(localSamplerInfo.MagFilter);
    samplerCreateInfo.mipmapMode = ConvertToVulkanSamplerMipMapMode(localSamplerInfo.MipFilter);
    samplerCreateInfo.addressModeU = ConvertToVulkanSamplerAddressMode(localSamplerInfo.AddressU);
    samplerCreateInfo.addressModeV = ConvertToVulkanSamplerAddressMode(localSamplerInfo.AddressV);
    samplerCreateInfo.addressModeW = ConvertToVulkanSamplerAddressMode(localSamplerInfo.AddressW);
    samplerCreateInfo.anisotropyEnable = localSamplerInfo.MaxAnisotropy > 1;
    samplerCreateInfo.maxAnisotropy = localSamplerInfo.MaxAnisotropy;
    samplerCreateInfo.compareEnable = localSamplerInfo.CompareFunction != ElemGraphicsCompareFunction_Never;
    samplerCreateInfo.compareOp = ConvertToVulkanCompareFunction(localSamplerInfo.CompareFunction);
    samplerCreateInfo.minLod = localSamplerInfo.MinLod;
    samplerCreateInfo.maxLod = localSamplerInfo.MaxLod == 0 ? VULKAN_MAX_MIPS : localSamplerInfo.MaxLod;
    samplerCreateInfo.borderColor = borderColor;

    VkSampler sampler;
	AssertIfFailed(vkCreateSampler(graphicsDeviceData->Device, &samplerCreateInfo, 0, &sampler));

    VkDescriptorImageInfo imageInfo = {};
    imageInfo.sampler = sampler;

    VkWriteDescriptorSet descriptor = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
    descriptor.dstSet = descriptorHeap.Storage->DescriptorSet->DescriptorSet;
    descriptor.dstBinding = 0;
    descriptor.dstArrayElement = descriptorHandle;
    descriptor.descriptorCount = 1;
    descriptor.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    descriptor.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(graphicsDeviceData->Device, 1, &descriptor, 0, nullptr);

    if ((descriptorHandle % 1024) == 0)
    {
        SystemCommitMemory(VulkanGraphicsMemoryArena, &vulkanSamplerInfos[descriptorHandle], 1024 *  sizeof(ElemGraphicsSamplerInfo));
    }

    vulkanSamplerInfos[descriptorHandle].GraphicsDevice = graphicsDevice;
    vulkanSamplerInfos[descriptorHandle].VulkanSampler = sampler;
    vulkanSamplerInfos[descriptorHandle].SamplerInfo = localSamplerInfo;
    return descriptorHandle;
}

ElemGraphicsSamplerInfo VulkanGetGraphicsSamplerInfo(ElemGraphicsSampler sampler)
{
    InitVulkanResourceMemory();

    if (sampler == -1)
    {
        SystemLogErrorMessage(ElemLogMessageCategory_Graphics, "Sampler is invalid.");
        return {};
    }

    return vulkanSamplerInfos[sampler].SamplerInfo;
}

void VulkanFreeGraphicsSampler(ElemGraphicsSampler sampler, const ElemFreeGraphicsSamplerOptions* options)
{
    InitVulkanResourceMemory();

    if (sampler == -1)
    {
        SystemLogErrorMessage(ElemLogMessageCategory_Graphics, "Sampler is invalid.");
        return;
    }
    
    if (options && options->FencesToWait.Length > 0)
    {
        EnqueueResourceDeleteEntry(VulkanGraphicsMemoryArena, sampler, ResourceDeleteType_Sampler, options->FencesToWait);
        return;
    }

    auto samplerInfo = vulkanSamplerInfos[sampler];

    if (samplerInfo.VulkanSampler != VK_NULL_HANDLE)
    {
        auto graphicsDeviceData = GetVulkanGraphicsDeviceData(samplerInfo.GraphicsDevice);
        SystemAssert(graphicsDeviceData);

        vkDestroySampler(graphicsDeviceData->Device, samplerInfo.VulkanSampler, nullptr);
        vulkanSamplerInfos[sampler] = {};
    }
}

ElemRaytracingAllocationInfo VulkanGetRaytracingBlasAllocationInfo(ElemGraphicsDevice graphicsDevice, const ElemRaytracingBlasParameters* parameters)
{
    auto stackMemoryArena = SystemGetStackMemoryArena();
    // TODO: Add validation
    
    auto graphicsDeviceData = GetVulkanGraphicsDeviceData(graphicsDevice);
    SystemAssert(graphicsDeviceData);

    auto geometryInfo = BuildVulkanBlasGeometryInfo(stackMemoryArena, parameters);
    auto triangleCount = parameters->IndexCount / 3;

    VkAccelerationStructureBuildSizesInfoKHR sizeInfo = { VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR };
	vkGetAccelerationStructureBuildSizesKHR(graphicsDeviceData->Device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_HOST_OR_DEVICE_KHR, &geometryInfo, &triangleCount, &sizeInfo);

    SystemLogDebugMessage(ElemLogMessageCategory_Graphics, "BLAS Size: %d", sizeInfo.accelerationStructureSize);
    SystemLogDebugMessage(ElemLogMessageCategory_Graphics, "Scratch Size: %d", sizeInfo.buildScratchSize);
    SystemLogDebugMessage(ElemLogMessageCategory_Graphics, "Update Size: %d", sizeInfo.updateScratchSize);

    return 
    {
        .Alignment = 256,
        .SizeInBytes = sizeInfo.accelerationStructureSize,
        .ScratchSizeInBytes = sizeInfo.buildScratchSize,
        .UpdateScratchSizeInBytes = sizeInfo.updateScratchSize
    };
}

ElemRaytracingAllocationInfo VulkanGetRaytracingTlasAllocationInfo(ElemGraphicsDevice graphicsDevice, const ElemRaytracingTlasParameters* parameters)
{
    auto stackMemoryArena = SystemGetStackMemoryArena();
    // TODO: Add validation
    
    auto graphicsDeviceData = GetVulkanGraphicsDeviceData(graphicsDevice);
    SystemAssert(graphicsDeviceData);

    auto geometryInfo = BuildVulkanTlasGeometryInfo(stackMemoryArena, parameters);

    VkAccelerationStructureBuildSizesInfoKHR sizeInfo = { VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR };
	vkGetAccelerationStructureBuildSizesKHR(graphicsDeviceData->Device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_HOST_OR_DEVICE_KHR, &geometryInfo, &parameters->InstanceCount, &sizeInfo);

    SystemLogDebugMessage(ElemLogMessageCategory_Graphics, "TLAS Size: %d", sizeInfo.accelerationStructureSize);
    SystemLogDebugMessage(ElemLogMessageCategory_Graphics, "Scratch Size: %d", sizeInfo.buildScratchSize);
    SystemLogDebugMessage(ElemLogMessageCategory_Graphics, "Update Size: %d", sizeInfo.updateScratchSize);

    return 
    {
        .Alignment = 256,
        .SizeInBytes = sizeInfo.accelerationStructureSize,
        .ScratchSizeInBytes = sizeInfo.buildScratchSize,
        .UpdateScratchSizeInBytes = sizeInfo.updateScratchSize
    };
}

ElemGraphicsResourceAllocationInfo VulkanGetRaytracingTlasInstanceAllocationInfo(ElemGraphicsDevice graphicsDevice, uint32_t instanceCount)
{
    return 
    {
        .Alignment = 4,
        .SizeInBytes = instanceCount * sizeof(VkAccelerationStructureInstanceKHR)
    };
}

ElemDataSpan VulkanEncodeRaytracingTlasInstances(ElemRaytracingTlasInstanceSpan instances)
{
    InitVulkanResourceMemory();

    auto result = SystemPushArray<VkAccelerationStructureInstanceKHR>(vulkanRaytracingInstanceMemoryArena, instances.Length);

    for (uint32_t i = 0; i < instances.Length; i++)
    {
        auto instance = &instances.Items[i];

        auto validVirtualAddress = false;
        uint64_t blasVirtualAddress = 0;

        if (instance->BlasResource)
        {
            auto blasResourceData = GetVulkanGraphicsResourceData(instance->BlasResource);
            SystemAssert(blasResourceData);

            auto blasResourceDataFull = GetVulkanGraphicsResourceDataFull(instance->BlasResource);
            SystemAssert(blasResourceDataFull);

            auto graphicsDeviceData = GetVulkanGraphicsDeviceData(blasResourceDataFull->GraphicsDevice);
            SystemAssert(graphicsDeviceData);

            if (blasResourceData->Type == ElemGraphicsResourceType_RaytracingAccelerationStructure)
            {
                validVirtualAddress = true;
    
                VkAccelerationStructureDeviceAddressInfoKHR deviceAddressInfo = { VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR };
                deviceAddressInfo.accelerationStructure = blasResourceData->AccelerationStructureDeviceObject;

                VkDeviceAddress address = vkGetAccelerationStructureDeviceAddressKHR(graphicsDeviceData->Device, &deviceAddressInfo);
                SystemAssert(address);
                blasVirtualAddress = address;
            }
        }

        if (!validVirtualAddress)
        {
            SystemLogErrorMessage(ElemLogMessageCategory_Graphics, "BlasResouce in Tlas instance should be an acceleration structure.");
            return {};
        }

        result[i] =
        {
            .instanceCustomIndex = instance->InstanceId,
            .mask = instance->InstanceMask,
            .flags = (uint32_t)ConvertToVulkanRaytracingInstanceFlags(instance->InstanceFlags),
            .accelerationStructureReference =  blasVirtualAddress
        };
                
        for (uint32_t j = 0; j < 3; j++)
        {
            for (uint32_t k = 0; k < 4; k++)
            {
                result[i].transform.matrix[j][k] = instance->TransformMatrix.Elements[k][j];
            }
        }
    }
    
    return { .Items = (uint8_t*)result.Pointer, .Length = (uint32_t)(result.Length * sizeof(VkAccelerationStructureInstanceKHR)) };
}

ElemGraphicsResource VulkanCreateRaytracingAccelerationStructureResource(ElemGraphicsDevice graphicsDevice, ElemGraphicsResource storageBuffer, const ElemRaytracingAccelerationStructureOptions* options)
{
    auto resourceData = GetVulkanGraphicsResourceData(storageBuffer);
    SystemAssert(resourceData);

    auto resourceDataFull = GetVulkanGraphicsResourceDataFull(storageBuffer);
    SystemAssert(resourceDataFull);

    auto graphicsDeviceData = GetVulkanGraphicsDeviceData(resourceDataFull->GraphicsDevice);
    SystemAssert(graphicsDeviceData);

    auto graphicsHeapData = GetVulkanGraphicsHeapData(resourceDataFull->GraphicsHeap);
    SystemAssert(graphicsHeapData);

    if (!(resourceData->Usage & ElemGraphicsResourceUsage_RaytracingAccelerationStructure))
    {
        SystemLogErrorMessage(ElemLogMessageCategory_Graphics, "RaytracingAccelerationStructure need to have a storage buffer that was created with RaytracingAccelerationStructure usage.");
        return ELEM_HANDLE_NULL;
    }

    auto storageBufferOffset = 0u;
    auto storageBufferSizeInBytes = resourceData->Width;

    if (options)
    {
        storageBufferOffset = options->StorageOffset;

        if (options->StorageSizeInBytes > 0)
        {
            storageBufferSizeInBytes = options->StorageSizeInBytes;
        }
    }

    VkAccelerationStructureCreateInfoKHR accelerationCreateInfo = { VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR };
    accelerationCreateInfo.buffer = resourceData->BufferDeviceObject;
    accelerationCreateInfo.offset = storageBufferOffset;
    accelerationCreateInfo.size = storageBufferSizeInBytes;
    accelerationCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_GENERIC_KHR;

    VkAccelerationStructureKHR accelerationStructure;
    AssertIfFailedReturnNullHandle(vkCreateAccelerationStructureKHR(graphicsDeviceData->Device, &accelerationCreateInfo, nullptr, &accelerationStructure));

    if (VulkanDebugLayerEnabled && options && options->DebugName)
    {
        VkDebugUtilsObjectNameInfoEXT nameInfo = { VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT };
        nameInfo.objectType = VK_OBJECT_TYPE_BUFFER;
        nameInfo.objectHandle = (uint64_t)accelerationStructure;
        nameInfo.pObjectName = options->DebugName;

        AssertIfFailed(vkSetDebugUtilsObjectNameEXT(graphicsDeviceData->Device, &nameInfo)); 
    }

    auto handle = SystemAddDataPoolItem(vulkanGraphicsResourcePool, {
        .AccelerationStructureDeviceObject = accelerationStructure,
        .Type = ElemGraphicsResourceType_RaytracingAccelerationStructure,
        .Width = storageBufferSizeInBytes,
        .Usage = resourceData->Usage
    }); 

    SystemAddDataPoolItemFull(vulkanGraphicsResourcePool, handle, {
        .GraphicsDevice = graphicsHeapData->GraphicsDevice,
        .GraphicsHeap = resourceDataFull->GraphicsHeap,
        .GraphicsHeapOffset = resourceDataFull->GraphicsHeapOffset + storageBufferOffset
    });

    return handle;
}

void VulkanBuildRaytracingBlas(ElemCommandList commandList, ElemGraphicsResource accelerationStructure, ElemGraphicsResource scratchBuffer, const ElemRaytracingBlasParameters* parameters, const ElemRaytracingBuildOptions* options)
{
    auto stackMemoryArena = SystemGetStackMemoryArena();
    
    // TODO: Add validation

    auto commandListData = GetVulkanCommandListData(commandList);
    SystemAssert(commandListData);

    auto graphicsDeviceData = GetVulkanGraphicsDeviceData(commandListData->GraphicsDevice);
    SystemAssert(graphicsDeviceData);

    auto accelerationStructureResourceData = GetVulkanGraphicsResourceData(accelerationStructure);
    SystemAssert(accelerationStructureResourceData);

    auto scratchBufferResourceData = GetVulkanGraphicsResourceData(scratchBuffer);
    SystemAssert(scratchBufferResourceData);
    
    if (accelerationStructureResourceData->Type != ElemGraphicsResourceType_RaytracingAccelerationStructure)
    {
        SystemLogErrorMessage(ElemLogMessageCategory_Graphics, "Acceleration structure is not an acceleration structure graphics resource.");
        return;
    }
    
    auto inputs = BuildVulkanBlasGeometryInfo(stackMemoryArena, parameters);

    auto scratchOffset = 0u;

    if (options)
    {
        scratchOffset = options->ScratchOffset;
    }

    VkBufferDeviceAddressInfo deviceAddressInfo = { VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO };
    deviceAddressInfo.buffer = scratchBufferResourceData->BufferDeviceObject;

    VkDeviceAddress address = vkGetBufferDeviceAddress(graphicsDeviceData->Device, &deviceAddressInfo);
    SystemAssert(address);

    inputs.scratchData.deviceAddress = address + scratchOffset;
    inputs.dstAccelerationStructure = accelerationStructureResourceData->AccelerationStructureDeviceObject;

    VkAccelerationStructureBuildRangeInfoKHR buildRanges =
    {
        .primitiveCount = parameters->IndexCount / 3
    };

    VkAccelerationStructureBuildRangeInfoKHR* buildRangesPtr = &buildRanges;

    InsertVulkanResourceBarriersIfNeeded(commandList, ElemGraphicsResourceBarrierSyncType_BuildRaytracingAccelerationStructure);
    vkCmdBuildAccelerationStructuresKHR(commandListData->DeviceObject, 1, &inputs, &buildRangesPtr);
}

void VulkanBuildRaytracingTlas(ElemCommandList commandList, ElemGraphicsResource accelerationStructure, ElemGraphicsResource scratchBuffer, const ElemRaytracingTlasParameters* parameters, const ElemRaytracingBuildOptions* options)
{
    auto stackMemoryArena = SystemGetStackMemoryArena();
    
    // TODO: Add validation

    auto commandListData = GetVulkanCommandListData(commandList);
    SystemAssert(commandListData);

    auto graphicsDeviceData = GetVulkanGraphicsDeviceData(commandListData->GraphicsDevice);
    SystemAssert(graphicsDeviceData);

    auto accelerationStructureResourceData = GetVulkanGraphicsResourceData(accelerationStructure);
    SystemAssert(accelerationStructureResourceData);

    auto scratchBufferResourceData = GetVulkanGraphicsResourceData(scratchBuffer);
    SystemAssert(scratchBufferResourceData);
    
    if (accelerationStructureResourceData->Type != ElemGraphicsResourceType_RaytracingAccelerationStructure)
    {
        SystemLogErrorMessage(ElemLogMessageCategory_Graphics, "Acceleration structure is not an acceleration structure graphics resource.");
        return;
    }
    
    auto inputs = BuildVulkanTlasGeometryInfo(stackMemoryArena, parameters);

    auto scratchOffset = 0u;

    if (options)
    {
        scratchOffset = options->ScratchOffset;
    }

    VkBufferDeviceAddressInfo deviceAddressInfo = { VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO };
    deviceAddressInfo.buffer = scratchBufferResourceData->BufferDeviceObject;

    VkDeviceAddress address = vkGetBufferDeviceAddress(graphicsDeviceData->Device, &deviceAddressInfo);
    SystemAssert(address);

    inputs.scratchData.deviceAddress = address + scratchOffset;
    inputs.dstAccelerationStructure = accelerationStructureResourceData->AccelerationStructureDeviceObject;

    VkAccelerationStructureBuildRangeInfoKHR buildRanges =
    {
        .primitiveCount = parameters->InstanceCount
    };

    VkAccelerationStructureBuildRangeInfoKHR* buildRangesPtr = &buildRanges;

    InsertVulkanResourceBarriersIfNeeded(commandList, ElemGraphicsResourceBarrierSyncType_BuildRaytracingAccelerationStructure);
    vkCmdBuildAccelerationStructuresKHR(commandListData->DeviceObject, 1, &inputs, &buildRangesPtr);
}
