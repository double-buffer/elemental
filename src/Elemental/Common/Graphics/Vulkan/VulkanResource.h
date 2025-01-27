#pragma once

#include "Elemental.h"
#include "volk.h"

struct VulkanGraphicsHeapData
{
    VkDeviceMemory DeviceObject;
    ElemGraphicsHeapType HeapType;
    uint64_t SizeInBytes;
    ElemGraphicsDevice GraphicsDevice;
};

// TODO: Review data usage!
struct VulkanGraphicsResourceData
{
    VkBuffer BufferDeviceObject;
    VkImage TextureDeviceObject;
    VkAccelerationStructureKHR AccelerationStructureDeviceObject;
    ElemGraphicsResourceType Type;
    VkImageView RenderTargetImageView;
    VkImageView DepthStencilImageView;
    VkFormat Format;
    ElemGraphicsFormat InternalFormat;
    bool IsPresentTexture;
    uint32_t Width;
    uint32_t Height;
    uint32_t MipLevels;
    ElemGraphicsResourceUsage Usage;
};

struct VulkanGraphicsResourceDataFull
{
    ElemGraphicsDevice GraphicsDevice;
    ElemGraphicsHeap GraphicsHeap;
    uint64_t GraphicsHeapOffset;
};

struct VulkanUploadBuffer
{
    VkBuffer Buffer;
    VkDeviceMemory DeviceMemory;
};

struct VulkanGraphicsSamplerInfo
{
    ElemGraphicsDevice GraphicsDevice;
    VkSampler VulkanSampler;
    ElemGraphicsSamplerInfo SamplerInfo;
};

VulkanGraphicsHeapData* GetVulkanGraphicsHeapData(ElemGraphicsHeap graphicsHeap);

VulkanGraphicsResourceData* GetVulkanGraphicsResourceData(ElemGraphicsResource resource);
VulkanGraphicsResourceDataFull* GetVulkanGraphicsResourceDataFull(ElemGraphicsResource resource);

VkFormat ConvertToVulkanTextureFormat(ElemGraphicsFormat format);
ElemGraphicsResource CreateVulkanTextureFromResource(ElemGraphicsDevice graphicsDevice, VkImage resource, const ElemGraphicsResourceInfo* resourceInfo, bool isPresentTexture);

ElemGraphicsHeap VulkanCreateGraphicsHeap(ElemGraphicsDevice graphicsDevice, uint64_t sizeInBytes, const ElemGraphicsHeapOptions* options);
void VulkanFreeGraphicsHeap(ElemGraphicsHeap graphicsHeap);
            
ElemGraphicsResourceInfo VulkanCreateGraphicsBufferResourceInfo(ElemGraphicsDevice graphicsDevice, uint32_t sizeInBytes, ElemGraphicsResourceUsage usage, const ElemGraphicsResourceInfoOptions* options);
ElemGraphicsResourceInfo VulkanCreateTexture2DResourceInfo(ElemGraphicsDevice graphicsDevice, uint32_t width, uint32_t height, uint32_t mipLevels, ElemGraphicsFormat format, ElemGraphicsResourceUsage usage, const ElemGraphicsResourceInfoOptions* options);

ElemGraphicsResource VulkanCreateGraphicsResource(ElemGraphicsHeap graphicsHeap, uint64_t graphicsHeapOffset, const ElemGraphicsResourceInfo* resourceInfo);
void VulkanFreeGraphicsResource(ElemGraphicsResource resource, const ElemFreeGraphicsResourceOptions* options);
ElemGraphicsResourceInfo VulkanGetGraphicsResourceInfo(ElemGraphicsResource resource);

void VulkanUploadGraphicsBufferData(ElemGraphicsResource buffer, uint32_t offset, ElemDataSpan data);
ElemDataSpan VulkanDownloadGraphicsBufferData(ElemGraphicsResource buffer, const ElemDownloadGraphicsBufferDataOptions* options);
void VulkanCopyDataToGraphicsResource(ElemCommandList commandList, const ElemCopyDataToGraphicsResourceParameters* parameters);

ElemGraphicsResourceDescriptor VulkanCreateGraphicsResourceDescriptor(ElemGraphicsResource resource, ElemGraphicsResourceDescriptorUsage usage, const ElemGraphicsResourceDescriptorOptions* options);
ElemGraphicsResourceDescriptorInfo VulkanGetGraphicsResourceDescriptorInfo(ElemGraphicsResourceDescriptor descriptor);
void VulkanFreeGraphicsResourceDescriptor(ElemGraphicsResourceDescriptor descriptor, const ElemFreeGraphicsResourceDescriptorOptions* options);

void VulkanProcessGraphicsResourceDeleteQueue(ElemGraphicsDevice graphicsDevice);

void VulkanGraphicsResourceBarrier(ElemCommandList commandList, ElemGraphicsResourceDescriptor descriptor, const ElemGraphicsResourceBarrierOptions* options);

ElemGraphicsSampler VulkanCreateGraphicsSampler(ElemGraphicsDevice graphicsDevice, const ElemGraphicsSamplerInfo* samplerInfo);
ElemGraphicsSamplerInfo VulkanGetGraphicsSamplerInfo(ElemGraphicsSampler sampler);
void VulkanFreeGraphicsSampler(ElemGraphicsSampler sampler, const ElemFreeGraphicsSamplerOptions* options);

ElemRaytracingAllocationInfo VulkanGetRaytracingBlasAllocationInfo(ElemGraphicsDevice graphicsDevice, const ElemRaytracingBlasParameters* parameters);
ElemRaytracingAllocationInfo VulkanGetRaytracingTlasAllocationInfo(ElemGraphicsDevice graphicsDevice, const ElemRaytracingTlasParameters* parameters);

ElemGraphicsResourceAllocationInfo VulkanGetRaytracingTlasInstanceAllocationInfo(ElemGraphicsDevice graphicsDevice, uint32_t instanceCount);
ElemDataSpan VulkanEncodeRaytracingTlasInstances(ElemRaytracingTlasInstanceSpan instances);

ElemGraphicsResource VulkanCreateRaytracingAccelerationStructureResource(ElemGraphicsDevice graphicsDevice, ElemGraphicsResource storageBuffer, const ElemRaytracingAccelerationStructureOptions* options);
void VulkanBuildRaytracingBlas(ElemCommandList commandList, ElemGraphicsResource accelerationStructure, ElemGraphicsResource scratchBuffer, const ElemRaytracingBlasParameters* parameters, const ElemRaytracingBuildOptions* options);
void VulkanBuildRaytracingTlas(ElemCommandList commandList, ElemGraphicsResource accelerationStructure, ElemGraphicsResource scratchBuffer, const ElemRaytracingTlasParameters* parameters, const ElemRaytracingBuildOptions* options);
