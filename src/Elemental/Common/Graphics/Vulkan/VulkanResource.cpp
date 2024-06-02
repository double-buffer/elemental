#include "VulkanResource.h"
#include "VulkanGraphicsDevice.h"
#include "SystemDataPool.h"
#include "SystemFunctions.h"
#include "SystemMemory.h"

#define VULKAN_MAX_GRAPHICSHEAP 32
#define VULKAN_MAX_TEXTURES UINT16_MAX

SystemDataPool<VulkanGraphicsHeapData, VulkanGraphicsHeapDataFull> vulkanGraphicsHeapPool;
SystemDataPool<VulkanTextureData, VulkanTextureDataFull> vulkanTexturePool;

void InitVulkanTextureMemory()
{
    if (!vulkanGraphicsHeapPool.Storage)
    {
        vulkanGraphicsHeapPool = SystemCreateDataPool<VulkanGraphicsHeapData, VulkanGraphicsHeapDataFull>(VulkanGraphicsMemoryArena, VULKAN_MAX_GRAPHICSHEAP);
        vulkanTexturePool = SystemCreateDataPool<VulkanTextureData, VulkanTextureDataFull>(VulkanGraphicsMemoryArena, VULKAN_MAX_TEXTURES);
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

VulkanTextureData* GetVulkanTextureData(ElemTexture texture)
{
    return SystemGetDataPoolItem(vulkanTexturePool, texture);
}

VulkanTextureDataFull* GetVulkanTextureDataFull(ElemTexture texture)
{
    return SystemGetDataPoolItemFull(vulkanTexturePool, texture);
}

ElemTexture CreateVulkanTextureFromResource(ElemGraphicsDevice graphicsDevice, VkImage resource, VkFormat format, uint32_t width, uint32_t height, bool isPresentTexture)
{
    InitVulkanTextureMemory();

    auto graphicsDeviceData = GetVulkanGraphicsDeviceData(graphicsDevice);
    SystemAssert(graphicsDeviceData);

    auto graphicsDeviceDataFull = GetVulkanGraphicsDeviceDataFull(graphicsDevice);
    SystemAssert(graphicsDeviceDataFull);

    VkImageViewCreateInfo createInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    createInfo.image = resource;
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format = format;
    createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    createInfo.subresourceRange.baseMipLevel = 0;
    createInfo.subresourceRange.levelCount = 1;
    createInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    AssertIfFailed(vkCreateImageView(graphicsDeviceData->Device, &createInfo, 0, &imageView));

    auto handle = SystemAddDataPoolItem(vulkanTexturePool, {
        .DeviceObject = resource,
        .ImageView = imageView,
        .Format = format,
        .IsPresentTexture = isPresentTexture,
        .Width = width,
        .Height = height
    }); 

    SystemAddDataPoolItemFull(vulkanTexturePool, handle, {
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

void VulkanBindGraphicsHeap(ElemCommandList commandList, ElemGraphicsHeap graphicsHeap)
{
}

ElemTexture VulkanCreateTexture(ElemGraphicsHeap graphicsHeap, uint64_t graphicsHeapOffset, const ElemTextureParameters* parameters)
{
    return {};
}

void VulkanFreeTexture(ElemTexture texture)
{
    // TODO: Do a kind a deferred texture delete so we don't crash if the resource is still in use
    SystemAssert(texture != ELEM_HANDLE_NULL);

    auto textureData = GetVulkanTextureData(texture);
    SystemAssert(textureData);

    auto textureDataFull = GetVulkanTextureDataFull(texture);
    SystemAssert(textureDataFull);

    auto graphicsDeviceData = GetVulkanGraphicsDeviceData(textureDataFull->GraphicsDevice);
    SystemAssert(graphicsDeviceData);

    vkDestroyImageView(graphicsDeviceData->Device, textureData->ImageView, nullptr);

    SystemRemoveDataPoolItem(vulkanTexturePool, texture);
}

ElemShaderDescriptor VulkanCreateTextureShaderDescriptor(ElemTexture texture, const ElemTextureShaderDescriptorOptions* options)
{
    return 0;
}

void VulkanFreeShaderDescriptor(ElemShaderDescriptor shaderDescriptor)
{
}
