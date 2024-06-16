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

struct VulkanGraphicsResourceData
{
    VkImage DeviceObject;
    VkImageView ImageView;
    VkFormat Format;
    bool IsPresentTexture;
    uint32_t Width;
    uint32_t Height;
};

struct VulkanGraphicsResourceDataFull
{
    ElemGraphicsDevice GraphicsDevice;
};

VulkanGraphicsHeapData* GetVulkanGraphicsHeapData(ElemGraphicsHeap graphicsHeap);
VulkanGraphicsHeapDataFull* GetVulkanGraphicsHeapDataFull(ElemGraphicsHeap graphicsHeap);

VulkanGraphicsResourceData* GetVulkanGraphicsResourceData(ElemGraphicsResource resource);
VulkanGraphicsResourceDataFull* GetVulkanGraphicsResourceDataFull(ElemGraphicsResource resource);

ElemGraphicsResource CreateVulkanTextureFromResource(ElemGraphicsDevice graphicsDevice, VkImage resource, VkFormat format, uint32_t width, uint32_t height, bool isPresentTexture);

ElemGraphicsHeap VulkanCreateGraphicsHeap(ElemGraphicsDevice graphicsDevice, uint64_t sizeInBytes, const ElemGraphicsHeapOptions* options);
void VulkanFreeGraphicsHeap(ElemGraphicsHeap graphicsHeap);
            
ElemGraphicsResource VulkanCreateGraphicsResource(ElemGraphicsHeap graphicsHeap, uint64_t graphicsHeapOffset, const ElemGraphicsResourceInfo* resourceInfo);
void VulkanFreeGraphicsResource(ElemGraphicsResource resource);

ElemShaderDescriptor VulkanCreateTextureShaderDescriptor(ElemGraphicsResource resource, const ElemTextureShaderDescriptorOptions* options);
void VulkanFreeShaderDescriptor(ElemShaderDescriptor shaderDescriptor);
