#include "VulkanRendering.h"
#include "VulkanCommandList.h"
#include "VulkanTexture.h"
#include "SystemLogging.h"
#include "SystemFunctions.h"

void VulkanBeginRenderPass(ElemCommandList commandList, const ElemBeginRenderPassParameters* parameters)
{
    // TODO: Check command list type != COMPUTE
    
    auto stackMemoryArena = SystemGetStackMemoryArena();

    SystemAssert(commandList != ELEM_HANDLE_NULL);
    SystemAssert(parameters);

    auto commandListData = GetVulkanCommandListData(commandList);
    SystemAssert(commandListData);

    auto commandListDataFull = GetVulkanCommandListDataFull(commandList);
    SystemAssert(commandListDataFull);
    commandListDataFull->CurrentRenderPassParameters = *parameters;

    for (uint32_t i = 0; i < parameters->RenderTargets.Length; i++)
    {
        auto renderTargetParameters = parameters->RenderTargets.Items[i];
        SystemAssert(renderTargetParameters.RenderTarget != ELEM_HANDLE_NULL);

        auto textureData = GetVulkanTextureData(renderTargetParameters.RenderTarget); 
        SystemAssert(textureData);

        //⚠️ : All barrier stuff will have a common logic and will try to maximize the grouping of barriers!!!
        // See: https://github.com/TheRealMJP/MemPoolTest/blob/8d7a5b9af5e6f1fe4ff3a35ba51aeb7924183ae2/SampleFramework12/v1.04/Graphics/GraphicsTypes.h#L461
        if (textureData->IsPresentTexture)
        {
            VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };

            barrier.srcAccessMask = VK_ACCESS_NONE_KHR;
            barrier.dstAccessMask = VK_ACCESS_NONE_KHR;
            barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = textureData->DeviceObject;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
            barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

            vkCmdPipelineBarrier(commandListData->DeviceObject, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, 1, &barrier);
        }
    }
}

void VulkanEndRenderPass(ElemCommandList commandList)
{
    SystemAssert(commandList != ELEM_HANDLE_NULL);

    auto commandListData = GetVulkanCommandListData(commandList);
    SystemAssert(commandListData);

    auto commandListDataFull = GetVulkanCommandListDataFull(commandList);
    SystemAssert(commandListDataFull);
    auto parameters = &commandListDataFull->CurrentRenderPassParameters;

    //commandListData->DeviceObject->EndRenderPass();

    for (uint32_t i = 0; i < parameters->RenderTargets.Length; i++)
    {
        auto renderTargetParameters = parameters->RenderTargets.Items[i];
        SystemAssert(renderTargetParameters.RenderTarget != ELEM_HANDLE_NULL);

        auto textureData = GetVulkanTextureData(renderTargetParameters.RenderTarget); 
        SystemAssert(textureData);

        // TODO: Handle texture accesses, currently we only handle the image layout
        // TODO: Use VkImageMemoryBarrier2. What are the differences?

        //⚠️ : All barrier stuff will have a common logic and will try to maximize the grouping of barriers!!!
        if (textureData->IsPresentTexture)
        {
            VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };

            barrier.srcAccessMask = VK_ACCESS_NONE_KHR;
            barrier.dstAccessMask = VK_ACCESS_NONE_KHR;
            barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = textureData->DeviceObject;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
            barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

            vkCmdPipelineBarrier(commandListData->DeviceObject, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
        }
    }
}

void VulkanSetViewports(ElemCommandList commandList, ElemViewportSpan viewports)
{
}

void VulkanDispatchMesh(ElemCommandList commandList, uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ)
{
}
