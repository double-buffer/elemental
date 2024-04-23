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

    auto renderingAttachments = SystemPushArray<VkRenderingAttachmentInfo>(stackMemoryArena, parameters->RenderTargets.Length);
    auto renderingWidth = 0u;
    auto renderingHeight = 0u;

    for (uint32_t i = 0; i < parameters->RenderTargets.Length; i++)
    {
        auto renderTargetParameters = parameters->RenderTargets.Items[i];
        SystemAssert(renderTargetParameters.RenderTarget != ELEM_HANDLE_NULL);

        auto textureData = GetVulkanTextureData(renderTargetParameters.RenderTarget); 
        SystemAssert(textureData);
        
        VkAttachmentLoadOp loadOperation;

        switch (renderTargetParameters.LoadAction)
        {
            case ElemRenderPassLoadAction_Load:
                loadOperation = VK_ATTACHMENT_LOAD_OP_LOAD;
                break;

            case ElemRenderPassLoadAction_Clear:
                loadOperation = VK_ATTACHMENT_LOAD_OP_CLEAR;
                break;

            default:
                loadOperation = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                break;
        }
        
        VkClearValue clearValue
        {
            .color = 
            {{ 
                renderTargetParameters.ClearColor.Red, 
                renderTargetParameters.ClearColor.Green, 
                renderTargetParameters.ClearColor.Blue, 
                renderTargetParameters.ClearColor.Alpha
            }}
        };

        VkAttachmentStoreOp storeOperation;

        switch (renderTargetParameters.StoreAction)
        {
            case ElemRenderPassStoreAction_Store:
                storeOperation = VK_ATTACHMENT_STORE_OP_STORE;
                break;

            default:
                storeOperation = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                break;
        }

        renderingAttachments[i] =
        {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = textureData->ImageView,
            .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
            .loadOp = loadOperation,
            .storeOp = storeOperation,
            .clearValue = clearValue
        };

        //⚠️ : All barrier stuff will have a common logic and will try to maximize the grouping of barriers!!!
        if (textureData->IsPresentTexture)
        {
            SystemLogDebugMessage(ElemLogMessageCategory_Graphics, "Barrier1");
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

        if (i == 0)
        {
            renderingWidth = textureData->Width;
            renderingHeight = textureData->Height;

            if (parameters->Viewports.Length == 0)
            {
                ElemViewport viewport =
                {
                    .X = 0, 
                    .Y = 0, 
                    .Width = (float)textureData->Width, 
                    .Height = (float)textureData->Height, 
                    .MinDepth = 0.0f, 
                    .MaxDepth = 1.0f
                };

                ElemSetViewport(commandList, &viewport); 
            }
        }
    }

    if (parameters->Viewports.Length > 0)
    {
        ElemSetViewports(commandList, parameters->Viewports);
    }
    
    // TODO: Take into account the depth buffer because it is possible to render only
    // to the depth buffer without any color render targets
    SystemAssert(renderingWidth != 0 && renderingHeight != 0);

    VkRenderingInfo renderingInfo = { VK_STRUCTURE_TYPE_RENDERING_INFO };
    renderingInfo.renderArea.extent.width = renderingWidth;
    renderingInfo.renderArea.extent.height = renderingHeight;
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = renderingAttachments.Length;
    renderingInfo.pColorAttachments = renderingAttachments.Pointer;

    vkCmdBeginRendering(commandListData->DeviceObject, &renderingInfo);
}

void VulkanEndRenderPass(ElemCommandList commandList)
{
    SystemAssert(commandList != ELEM_HANDLE_NULL);

    auto commandListData = GetVulkanCommandListData(commandList);
    SystemAssert(commandListData);

    auto commandListDataFull = GetVulkanCommandListDataFull(commandList);
    SystemAssert(commandListDataFull);
    auto parameters = &commandListDataFull->CurrentRenderPassParameters;

    vkCmdEndRendering(commandListData->DeviceObject);

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
    // TODO: Check command list type != COMPUTE

    SystemAssert(commandList != ELEM_HANDLE_NULL);

    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto vulkanViewports = SystemPushArray<VkViewport>(stackMemoryArena, viewports.Length);
    auto vulkanScissorRects = SystemPushArray<VkRect2D>(stackMemoryArena, viewports.Length);

    for (uint32_t i = 0; i < viewports.Length; i++)
    {
        vulkanViewports[i] = 
        {
            .x = viewports.Items[i].X,
            .y = viewports.Items[i].Y,
            .width = viewports.Items[i].Width,
            .height = viewports.Items[i].Height,
            .minDepth = viewports.Items[i].MinDepth,
            .maxDepth = viewports.Items[i].MaxDepth
        };

        vulkanScissorRects[i] = 
        {
            .offset = 
            {
                .x = (int32_t)viewports.Items[i].X,
                .y = (int32_t)viewports.Items[i].Y,
            },
            .extent = 
            {
                .width = (uint32_t)viewports.Items[i].Width,
                .height = (uint32_t)viewports.Items[i].Height
            }
        };
    }

    auto commandListData = GetVulkanCommandListData(commandList);
    SystemAssert(commandListData);

    vkCmdSetViewport(commandListData->DeviceObject, 0, vulkanViewports.Length, vulkanViewports.Pointer);
    vkCmdSetScissor(commandListData->DeviceObject, 0, vulkanScissorRects.Length, vulkanScissorRects.Pointer);
}

void VulkanDispatchMesh(ElemCommandList commandList, uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ)
{
}
