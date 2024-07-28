#include "VulkanRendering.h"
#include "VulkanGraphicsDevice.h"
#include "VulkanCommandList.h"
#include "VulkanResource.h"
#include "VulkanResourceBarrier.h"
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

        auto textureData = GetVulkanGraphicsResourceData(renderTargetParameters.RenderTarget); 
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
            .imageView = textureData->RenderTargetImageView,
            .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
            .loadOp = loadOperation,
            .storeOp = storeOperation,
            .clearValue = clearValue
        };
        
        ResourceBarrierItem resourceBarrier =
        {
            .Type = ElemGraphicsResourceType_Texture2D,
            .Resource = renderTargetParameters.RenderTarget,
            .AfterAccess = ElemGraphicsResourceBarrierAccessType_RenderTarget,
            .AfterLayout = ElemGraphicsResourceBarrierLayoutType_RenderTarget
        };

        EnqueueBarrier(commandListData->ResourceBarrierPool, &resourceBarrier);

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

    InsertVulkanResourceBarriersIfNeeded(commandList, ElemGraphicsResourceBarrierSyncType_RenderTarget);
    vkCmdBeginRendering(commandListData->DeviceObject, &renderingInfo);
}

void VulkanEndRenderPass(ElemCommandList commandList)
{
    SystemAssert(commandList != ELEM_HANDLE_NULL);

    auto commandListData = GetVulkanCommandListData(commandList);
    SystemAssert(commandListData);

    auto commandListDataFull = GetVulkanCommandListDataFull(commandList);
    SystemAssert(commandListDataFull);

    auto graphicsDeviceData = GetVulkanGraphicsDeviceData(commandListData->GraphicsDevice);
    SystemAssert(graphicsDeviceData);

    auto commandQueueData = GetVulkanCommandQueueData(commandListData->CommandQueue);
    SystemAssert(commandQueueData);

    auto parameters = &commandListDataFull->CurrentRenderPassParameters;

    vkCmdEndRendering(commandListData->DeviceObject);

    for (uint32_t i = 0; i < parameters->RenderTargets.Length; i++)
    {
        auto renderTargetParameters = parameters->RenderTargets.Items[i];
        SystemAssert(renderTargetParameters.RenderTarget != ELEM_HANDLE_NULL);

        auto textureData = GetVulkanGraphicsResourceData(renderTargetParameters.RenderTarget); 
        SystemAssert(textureData);

        if (textureData->IsPresentTexture)
        {
            ResourceBarrierItem resourceBarrier =
            {
                .Type = ElemGraphicsResourceType_Texture2D,
                .Resource = renderTargetParameters.RenderTarget,
                .AfterAccess = ElemGraphicsResourceBarrierAccessType_NoAccess,
                .AfterLayout = ElemGraphicsResourceBarrierLayoutType_Present
            };

            EnqueueBarrier(commandListData->ResourceBarrierPool, &resourceBarrier);

            commandQueueData->SignalPresentSemaphore = true;
        }
    }
    
    InsertVulkanResourceBarriersIfNeeded(commandList, ElemGraphicsResourceBarrierSyncType_None);
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
            .y = viewports.Items[i].Y + viewports.Items[i].Height,
            .width = viewports.Items[i].Width,
            .height = -viewports.Items[i].Height,
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
    // TODO: Check command list type != COMPUTE

    SystemAssert(commandList != ELEM_HANDLE_NULL);

    auto commandListData = GetVulkanCommandListData(commandList);
    SystemAssert(commandListData);

    vkCmdDrawMeshTasksEXT(commandListData->DeviceObject, threadGroupCountX, threadGroupCountY, threadGroupCountZ);
}
