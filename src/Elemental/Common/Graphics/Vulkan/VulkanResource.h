#pragma once

#include "Elemental.h"
#include "volk.h"

struct VulkanGraphicsHeapData
{
    ComPtr<ID3D12Heap1> DeviceObject;
};

struct VulkanGraphicsHeapDataFull
{
    ElemGraphicsDevice GraphicsDevice;
};

struct VulkanTextureData
{
    VkImage DeviceObject;
    VkImageView ImageView;
    VkFormat Format;
    bool IsPresentTexture;
    uint32_t Width;
    uint32_t Height;
};

struct VulkanTextureDataFull
{
    ElemGraphicsDevice GraphicsDevice;
};

VulkanGraphicsHeapData* GetVulkanGraphicsHeapData(ElemGraphicsHeap graphicsHeap);
VulkanGraphicsHeapDataFull* GetVulkanGraphicsHeapDataFull(ElemGraphicsHeap graphicsHeap);

VulkanTextureData* GetVulkanTextureData(ElemTexture texture);
VulkanTextureDataFull* GetVulkanTextureDataFull(ElemTexture texture);

ElemTexture CreateVulkanTextureFromResource(ElemGraphicsDevice graphicsDevice, VkImage resource, VkFormat format, uint32_t width, uint32_t height, bool isPresentTexture);

ElemGraphicsHeap VulkanCreateGraphicsHeap(ElemGraphicsDevice graphicsDevice, uint64_t sizeInBytes, const ElemGraphicsHeapOptions* options);
void VulkanFreeGraphicsHeap(ElemGraphicsHeap graphicsHeap);
            
ElemTexture VulkanCreateTexture(ElemGraphicsHeap graphicsHeap, uint64_t graphicsHeapOffset, const ElemTextureParameters* parameters);
void VulkanFreeTexture(ElemTexture texture);

ElemShaderDescriptor VulkanCreateTextureShaderDescriptor(ElemTexture texture, const ElemTextureShaderDescriptorOptions* options);
void VulkanFreeShaderDescriptor(ElemShaderDescriptor shaderDescriptor);
