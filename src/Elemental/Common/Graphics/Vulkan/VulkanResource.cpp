#include "VulkanResource.h"
#include "VulkanGraphicsDevice.h"
#include "SystemDataPool.h"
#include "SystemFunctions.h"
#include "SystemMemory.h"

#define VULKAN_MAX_GRAPHICSHEAP 32

SystemDataPool<VulkanGraphicsHeapData, VulkanGraphicsHeapDataFull> vulkanGraphicsHeapPool;
SystemDataPool<VulkanGraphicsResourceData, VulkanGraphicsResourceDataFull> vulkanGraphicsResourcePool;

void InitVulkanTextureMemory()
{
    if (!vulkanGraphicsHeapPool.Storage)
    {
        vulkanGraphicsHeapPool = SystemCreateDataPool<VulkanGraphicsHeapData, VulkanGraphicsHeapDataFull>(VulkanGraphicsMemoryArena, VULKAN_MAX_GRAPHICSHEAP);
        vulkanGraphicsResourcePool = SystemCreateDataPool<VulkanGraphicsResourceData, VulkanGraphicsResourceDataFull>(VulkanGraphicsMemoryArena, VULKAN_MAX_RESOURCES);
    }
}

VulkanGraphicsHeapData* GetVulkanGraphicsHeapData(ElemGraphicsHeap graphicsHeap)
{
    return SystemGetDataPoolItem(vulkanGraphicsHeapPool, graphicsHeap);
}

VulkanGraphicsHeapDataFull* GetVulkanGraphicsHeapDataFull(ElemGraphicsHeap graphicsHeap)
{
    return SystemGetDataPoolItemFull(vulkanGraphicsHeapPool, graphicsHeap);
}

VulkanGraphicsResourceData* GetVulkanGraphicsResourceData(ElemGraphicsResource texture)
{
    return SystemGetDataPoolItem(vulkanGraphicsResourcePool, texture);
}

VulkanGraphicsResourceDataFull* GetVulkanGraphicsResourceDataFull(ElemGraphicsResource texture)
{
    return SystemGetDataPoolItemFull(vulkanGraphicsResourcePool, texture);
}

ElemGraphicsResource CreateVulkanTextureFromResource(ElemGraphicsDevice graphicsDevice, VkImage resource, VkFormat format, uint32_t width, uint32_t height, bool isPresentTexture)
{
    InitVulkanTextureMemory();

    auto graphicsDeviceData = GetVulkanGraphicsDeviceData(graphicsDevice);
    SystemAssert(graphicsDeviceData);

    auto graphicsDeviceDataFull = GetVulkanGraphicsDeviceDataFull(graphicsDevice);
    SystemAssert(graphicsDeviceDataFull);


    auto handle = SystemAddDataPoolItem(vulkanGraphicsResourcePool, {
        .DeviceObject = resource,
        .Format = format,
        .IsPresentTexture = isPresentTexture,
        .Width = width,
        .Height = height
    }); 

    SystemAddDataPoolItemFull(vulkanGraphicsResourcePool, handle, {
        .GraphicsDevice = graphicsDevice
    });

    return handle;
}

// TODO: Functions to create additional optional Descriptors

ElemGraphicsHeap VulkanCreateGraphicsHeap(ElemGraphicsDevice graphicsDevice, uint64_t sizeInBytes, const ElemGraphicsHeapOptions* options)
{
    return {};
}

void VulkanFreeGraphicsHeap(ElemGraphicsHeap graphicsHeap)
{
}

ElemGraphicsResourceInfo VulkanCreateGraphicsBufferResourceInfo(ElemGraphicsDevice graphicsDevice, uint32_t sizeInBytes, ElemGraphicsResourceUsage usage, const ElemGraphicsResourceInfoOptions* options)
{
    return {};
}

ElemGraphicsResourceInfo VulkanCreateTexture2DResourceInfo(ElemGraphicsDevice graphicsDevice, uint32_t width, uint32_t height, uint32_t mipLevels, ElemGraphicsFormat format, ElemGraphicsResourceUsage usage, const ElemGraphicsResourceInfoOptions* options)
{
    return {};
}

ElemGraphicsResource VulkanCreateGraphicsResource(ElemGraphicsHeap graphicsHeap, uint64_t graphicsHeapOffset, const ElemGraphicsResourceInfo* resourceInfo)
{
    return {};
}

void VulkanFreeGraphicsResource(ElemGraphicsResource resource, const ElemFreeGraphicsResourceOptions* options)
{
    // TODO: Do a kind a deferred resource delete so we don't crash if the resource is still in use
    SystemAssert(resource != ELEM_HANDLE_NULL);

    auto resourceData = GetVulkanGraphicsResourceData(resource);
    SystemAssert(resourceData);

    auto resourceDataFull = GetVulkanGraphicsResourceDataFull(resource);
    SystemAssert(resourceDataFull);

    auto graphicsDeviceData = GetVulkanGraphicsDeviceData(resourceDataFull->GraphicsDevice);
    SystemAssert(graphicsDeviceData);

    SystemRemoveDataPoolItem(vulkanGraphicsResourcePool, resource);
}

ElemGraphicsResourceInfo VulkanGetGraphicsResourceInfo(ElemGraphicsResource resource)
{
    return {};
}

ElemDataSpan VulkanGetGraphicsResourceDataSpan(ElemGraphicsResource resource)
{
    return {};
}

ElemGraphicsResourceDescriptor VulkanCreateGraphicsResourceDescriptor(ElemGraphicsResource resource, ElemGraphicsResourceDescriptorUsage usage, const ElemGraphicsResourceDescriptorOptions* options)
{
/*    VkImageViewCreateInfo createInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    createInfo.image = resource;
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format = format;
    createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    createInfo.subresourceRange.baseMipLevel = 0;
    createInfo.subresourceRange.levelCount = 1;
    createInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    AssertIfFailed(vkCreateImageView(graphicsDeviceData->Device, &createInfo, 0, &imageView));*/
    return {};
}

ElemGraphicsResourceDescriptorInfo VulkanGetGraphicsResourceDescriptorInfo(ElemGraphicsResourceDescriptor descriptor)
{
    return {};
}

void VulkanFreeGraphicsResourceDescriptor(ElemGraphicsResourceDescriptor descriptor, const ElemFreeGraphicsResourceDescriptorOptions* options)
{
    //vkDestroyImageView(graphicsDeviceData->Device, resourceData->ImageView, nullptr);
}

void VulkanProcessGraphicsResourceDeleteQueue(void)
{
}

void VulkanGraphicsResourceBarrier(ElemCommandList commandList, ElemGraphicsResourceDescriptor descriptor, const ElemGraphicsResourceBarrierOptions* options)
{
}
