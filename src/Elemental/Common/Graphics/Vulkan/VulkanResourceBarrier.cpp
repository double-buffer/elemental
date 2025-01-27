#include "VulkanResource.h"
#include "VulkanGraphicsDevice.h"
#include "VulkanCommandList.h"
#include "SystemFunctions.h"
#include "SystemMemory.h"

VkPipelineStageFlags2 ConvertToVulkanBarrierSync(ElemGraphicsResourceBarrierSyncType syncType, bool isDepthStencil)
{
    switch (syncType) 
    {
        case ElemGraphicsResourceBarrierSyncType_None:
            return VK_PIPELINE_STAGE_2_NONE;

        case ElemGraphicsResourceBarrierSyncType_Compute:
            return VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;

        case ElemGraphicsResourceBarrierSyncType_Copy:
            return VK_PIPELINE_STAGE_2_COPY_BIT;

        case ElemGraphicsResourceBarrierSyncType_BuildRaytracingAccelerationStructure:
            return VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR;

        case ElemGraphicsResourceBarrierSyncType_RenderTarget:
        {
            if (!isDepthStencil)
            {
                return VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT | 
                       VK_PIPELINE_STAGE_2_MESH_SHADER_BIT_EXT | 
                       VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
            }
            else
            {
                return VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT |
                       VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
            }
        }
     }
}

VkAccessFlags2 ConvertToVulkanBarrierAccess(ElemGraphicsResourceBarrierAccessType accessType, bool isAccelerationStructure)
{
    // TODO: Recheck the correct accesses
    // Maybe we can pass more info to the function to compute more precides accesses (or in the common code)

    switch (accessType) 
    {
        case ElemGraphicsResourceBarrierAccessType_NoAccess:
            return VK_ACCESS_2_NONE;

        case ElemGraphicsResourceBarrierAccessType_Read:
            return isAccelerationStructure ? VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR : VK_ACCESS_2_SHADER_READ_BIT;

        case ElemGraphicsResourceBarrierAccessType_RenderTarget:
            return VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;

        case ElemGraphicsResourceBarrierAccessType_DepthStencilWrite:
            return VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        case ElemGraphicsResourceBarrierAccessType_Write:
            return VK_ACCESS_2_SHADER_WRITE_BIT;

        case ElemGraphicsResourceBarrierAccessType_Copy:
            return VK_ACCESS_2_TRANSFER_WRITE_BIT;
    }
}

VkImageLayout ConvertToVulkanBarrierLayout(ElemGraphicsResourceBarrierLayoutType layoutType)
{
    // TODO: Recheck the correct layouts
    // Maybe we can pass more info to the function to compute more precides layouts (or in the common code)
    // It would be better for common layout to specialize that base on the current queue type???

    switch (layoutType) 
    {
        case ElemGraphicsResourceBarrierLayoutType_Undefined:
            return VK_IMAGE_LAYOUT_UNDEFINED;

        case ElemGraphicsResourceBarrierLayoutType_Read:
            return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        case ElemGraphicsResourceBarrierLayoutType_Write:
            return VK_IMAGE_LAYOUT_GENERAL; // TODO: Not sure about this

        case ElemGraphicsResourceBarrierLayoutType_RenderTarget:
            return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        case ElemGraphicsResourceBarrierLayoutType_DepthStencilWrite:
            return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        case ElemGraphicsResourceBarrierLayoutType_Present:
            return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    }
}

void InsertVulkanResourceBarriersIfNeeded(ElemCommandList commandList, ElemGraphicsResourceBarrierSyncType currentStage)
{
    auto stackMemoryArena = SystemGetStackMemoryArena();
    
    SystemAssert(commandList != ELEM_HANDLE_NULL);

    auto commandListData = GetVulkanCommandListData(commandList);
    SystemAssert(commandListData);
    
    auto barriersInfo = GenerateBarrierCommands(stackMemoryArena, commandListData->ResourceBarrierPool, currentStage, VulkanDebugBarrierInfoEnabled);

    if (barriersInfo.BufferBarriers.Length == 0 && barriersInfo.TextureBarriers.Length == 0)
    {
        return;
    }
            
    VkDependencyInfo dependencyInfo = { VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
    dependencyInfo.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    if (barriersInfo.BufferBarriers.Length > 0)
    {
        auto vulkanBufferBarriers = SystemPushArray<VkBufferMemoryBarrier2>(stackMemoryArena, barriersInfo.BufferBarriers.Length);
        dependencyInfo.pBufferMemoryBarriers = vulkanBufferBarriers.Pointer;
        dependencyInfo.bufferMemoryBarrierCount = vulkanBufferBarriers.Length;

        for (uint32_t i = 0; i < barriersInfo.BufferBarriers.Length; i++)
        {
            auto barrier = barriersInfo.BufferBarriers[i];
            auto vulkanBufferBarrier = &vulkanBufferBarriers[i];

            auto graphicsResourceData = GetVulkanGraphicsResourceData(barrier.Resource);
            SystemAssert(graphicsResourceData);

            vulkanBufferBarrier->sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
            vulkanBufferBarrier->buffer = graphicsResourceData->BufferDeviceObject;
            vulkanBufferBarrier->size = graphicsResourceData->Width;
            vulkanBufferBarrier->srcStageMask = ConvertToVulkanBarrierSync(barrier.BeforeSync, false);
            vulkanBufferBarrier->dstStageMask = ConvertToVulkanBarrierSync(barrier.AfterSync, false);
            vulkanBufferBarrier->srcAccessMask = ConvertToVulkanBarrierAccess(barrier.BeforeAccess, graphicsResourceData->Usage & ElemGraphicsResourceUsage_RaytracingAccelerationStructure);
            vulkanBufferBarrier->dstAccessMask = ConvertToVulkanBarrierAccess(barrier.AfterAccess, graphicsResourceData->Usage & ElemGraphicsResourceUsage_RaytracingAccelerationStructure);
        }
    }

    if (barriersInfo.TextureBarriers.Length > 0)
    {
        auto vulkanTextureBarriers = SystemPushArray<VkImageMemoryBarrier2>(stackMemoryArena, barriersInfo.TextureBarriers.Length);
        dependencyInfo.pImageMemoryBarriers = vulkanTextureBarriers.Pointer;
        dependencyInfo.imageMemoryBarrierCount = vulkanTextureBarriers.Length;

        for (uint32_t i = 0; i < barriersInfo.TextureBarriers.Length; i++)
        {
            auto barrier = barriersInfo.TextureBarriers[i];
            auto vulkanTextureBarrier = &vulkanTextureBarriers[i];

            auto graphicsResourceData = GetVulkanGraphicsResourceData(barrier.Resource);
            SystemAssert(graphicsResourceData);

            vulkanTextureBarrier->sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
            vulkanTextureBarrier->image = graphicsResourceData->TextureDeviceObject;
            vulkanTextureBarrier->srcStageMask = ConvertToVulkanBarrierSync(barrier.BeforeSync, barrier.IsDepthStencil);
            vulkanTextureBarrier->dstStageMask = ConvertToVulkanBarrierSync(barrier.AfterSync, barrier.IsDepthStencil);
            vulkanTextureBarrier->srcAccessMask = ConvertToVulkanBarrierAccess(barrier.BeforeAccess, false);
            vulkanTextureBarrier->dstAccessMask = ConvertToVulkanBarrierAccess(barrier.AfterAccess, false);
            vulkanTextureBarrier->oldLayout = ConvertToVulkanBarrierLayout(barrier.BeforeLayout);
            vulkanTextureBarrier->newLayout = ConvertToVulkanBarrierLayout(barrier.AfterLayout);
            vulkanTextureBarrier->subresourceRange.aspectMask = graphicsResourceData->Usage & ElemGraphicsResourceUsage_DepthStencil ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
            vulkanTextureBarrier->subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
            vulkanTextureBarrier->subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
        }
    }

    vkCmdPipelineBarrier2(commandListData->DeviceObject, &dependencyInfo);
}

void VulkanGraphicsResourceBarrier(ElemCommandList commandList, ElemGraphicsResourceDescriptor descriptor, const ElemGraphicsResourceBarrierOptions* options)
{
    SystemAssert(commandList != ELEM_HANDLE_NULL);

    auto commandListData = GetVulkanCommandListData(commandList);
    SystemAssert(commandListData);

    EnqueueBarrier(commandListData->ResourceBarrierPool, descriptor, options);
}
