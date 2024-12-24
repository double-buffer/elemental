#include "VulkanResource.h"
#include "VulkanGraphicsDevice.h"
#include "Graphics/Resource.h"
#include "Graphics/ResourceDeleteQueue.h"
#include "SystemDataPool.h"
#include "SystemFunctions.h"
#include "SystemMemory.h"

#define VULKAN_MAX_GRAPHICSHEAP 32

SystemDataPool<VulkanGraphicsHeapData, SystemDataPoolDefaultFull> vulkanGraphicsHeapPool;
SystemDataPool<VulkanGraphicsResourceData, VulkanGraphicsResourceDataFull> vulkanGraphicsResourcePool;

// TODO: This descriptor infos should be linked to the graphics device like the resource desc heaps
Span<ElemGraphicsResourceDescriptorInfo> vulkanResourceDescriptorInfos;
// TODO: To refactor 
Span<VkImageView> vulkanResourceDescriptorImageViews;

void InitVulkanResourceMemory()
{
    if (!vulkanGraphicsHeapPool.Storage)
    {
        vulkanGraphicsHeapPool = SystemCreateDataPool<VulkanGraphicsHeapData, SystemDataPoolDefaultFull>(VulkanGraphicsMemoryArena, VULKAN_MAX_GRAPHICSHEAP);
        vulkanGraphicsResourcePool = SystemCreateDataPool<VulkanGraphicsResourceData, VulkanGraphicsResourceDataFull>(VulkanGraphicsMemoryArena, VULKAN_MAX_RESOURCES);
        vulkanResourceDescriptorInfos = SystemPushArray<ElemGraphicsResourceDescriptorInfo>(VulkanGraphicsMemoryArena, VULKAN_MAX_RESOURCES);
        vulkanResourceDescriptorImageViews = SystemPushArray<VkImageView>(VulkanGraphicsMemoryArena, VULKAN_MAX_RESOURCES);
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

        case ElemGraphicsFormat_B8G8R8A8_UNORM:
            return VK_FORMAT_B8G8R8A8_UNORM;

        case ElemGraphicsFormat_R16G16B16A16_FLOAT:
            return VK_FORMAT_R16G16B16A16_SFLOAT;

        case ElemGraphicsFormat_R32G32B32A32_FLOAT:
            return VK_FORMAT_R32G32B32A32_SFLOAT;
        
        case ElemGraphicsFormat_D32_FLOAT:
            return VK_FORMAT_D32_SFLOAT;

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

VkBuffer CreateVulkanBuffer(ElemGraphicsDevice graphicsDevice, const ElemGraphicsResourceInfo* resourceInfo)
{
    SystemAssert(graphicsDevice != ELEM_HANDLE_NULL);
    SystemAssert(resourceInfo);

    auto graphicsDeviceData = GetVulkanGraphicsDeviceData(graphicsDevice);
    SystemAssert(graphicsDeviceData);

    VkBufferCreateInfo createInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    createInfo.size = resourceInfo->Width;
    createInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

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

    VkMemoryAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    allocateInfo.allocationSize = sizeInBytes;
    // TODO: Still need to investigate this
    //allocateInfo.memoryTypeIndex = graphicsDeviceDataFull->GpuMemoryTypeIndex;
    allocateInfo.memoryTypeIndex = graphicsDeviceDataFull->GpuUploadMemoryTypeIndex;

    if (options)
    {
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

    auto buffer = CreateVulkanBuffer(graphicsDevice, &resourceInfo);

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

        auto buffer = CreateVulkanBuffer(graphicsHeapData->GraphicsDevice, resourceInfo);

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

ElemDataSpan VulkanGetGraphicsResourceDataSpan(ElemGraphicsResource resource)
{
    SystemAssert(resource != ELEM_HANDLE_NULL);

    auto resourceData = GetVulkanGraphicsResourceData(resource);
    SystemAssert(resourceData);

    if (resourceData->Type != ElemGraphicsResourceType_Buffer)
    {
        SystemLogErrorMessage(ElemLogMessageCategory_Graphics, "GetGraphicsResourceDataSpan only works with graphics buffers.");
        return {};
    }

	if (resourceData->CpuDataPointer == nullptr)
	{
        auto resourceDataFull = GetVulkanGraphicsResourceDataFull(resource);
        SystemAssert(resourceDataFull);

        auto graphicsDeviceData = GetVulkanGraphicsDeviceData(resourceDataFull->GraphicsDevice);
        SystemAssert(graphicsDeviceData);

        auto graphicsHeapData = GetVulkanGraphicsHeapData(resourceDataFull->GraphicsHeap);
        SystemAssert(graphicsHeapData);

        AssertIfFailed(vkMapMemory(graphicsDeviceData->Device, graphicsHeapData->DeviceObject, resourceDataFull->GraphicsHeapOffset, resourceData->Width, 0, &resourceData->CpuDataPointer));
	}

	return { .Items = (uint8_t*)resourceData->CpuDataPointer, .Length = resourceData->Width };
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

    VkWriteDescriptorSet descriptor = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
    descriptor.dstSet = descriptorHeap.Storage->DescriptorSet;
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

void VulkanProcessGraphicsResourceDeleteQueue()
{
    ProcessResourceDeleteQueue();
}
