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
            
ElemGraphicsResourceInfo VulkanCreateGraphicsBufferResourceInfo(ElemGraphicsDevice graphicsDevice, uint32_t sizeInBytes, ElemGraphicsResourceUsage usage, const ElemGraphicsResourceInfoOptions* options);
ElemGraphicsResourceInfo VulkanCreateTexture2DResourceInfo(ElemGraphicsDevice graphicsDevice, uint32_t width, uint32_t height, uint32_t mipLevels, ElemGraphicsFormat format, ElemGraphicsResourceUsage usage, const ElemGraphicsResourceInfoOptions* options);

ElemGraphicsResource VulkanCreateGraphicsResource(ElemGraphicsHeap graphicsHeap, uint64_t graphicsHeapOffset, const ElemGraphicsResourceInfo* resourceInfo);
void VulkanFreeGraphicsResource(ElemGraphicsResource resource, const ElemFreeGraphicsResourceOptions* options);
ElemGraphicsResourceInfo VulkanGetGraphicsResourceInfo(ElemGraphicsResource resource);
ElemDataSpan VulkanGetGraphicsResourceDataSpan(ElemGraphicsResource resource);

ElemGraphicsResourceDescriptor VulkanCreateGraphicsResourceDescriptor(ElemGraphicsResource resource, ElemGraphicsResourceDescriptorUsage usage, const ElemGraphicsResourceDescriptorOptions* options);
ElemGraphicsResourceDescriptorInfo VulkanGetGraphicsResourceDescriptorInfo(ElemGraphicsResourceDescriptor descriptor);
void VulkanFreeGraphicsResourceDescriptor(ElemGraphicsResourceDescriptor descriptor, const ElemFreeGraphicsResourceDescriptorOptions* options);

void VulkanProcessGraphicsResourceDeleteQueue(void);

void VulkanGraphicsResourceBarrier(ElemCommandList commandList, ElemGraphicsResourceDescriptor descriptor, const ElemGraphicsResourceBarrierOptions* options);
