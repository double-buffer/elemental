#include "MetalRendering.h"
#include "MetalCommandList.h"
#include "MetalShader.h"
#include "MetalResource.h"
#include "MetalResourceBarrier.h"
#include "SystemFunctions.h"
#include "SystemLogging.h"

void MetalBeginRenderPass(ElemCommandList commandList, const ElemBeginRenderPassParameters* parameters)
{
    // TODO: Check command list type != COMPUTE

    auto stackMemoryArena = SystemGetStackMemoryArena();

    SystemAssert(commandList != ELEM_HANDLE_NULL);
    SystemAssert(parameters);

    ResetMetalCommandEncoder(commandList);

    auto commandListData = GetMetalCommandListData(commandList);
    SystemAssert(commandListData);

    auto commandListDataFull = GetMetalCommandListDataFull(commandList);
    SystemAssert(commandListDataFull);

    auto renderPassDescriptor = NS::TransferPtr(MTL::RenderPassDescriptor::alloc()->init());

    for (uint32_t i = 0; i < parameters->RenderTargets.Length; i++)
    {
        auto renderTargetParameters = parameters->RenderTargets.Items[i];

        auto textureData = GetMetalResourceData(renderTargetParameters.RenderTarget); 
        SystemAssert(textureData);

        auto renderTargetDescriptor = renderPassDescriptor->colorAttachments()->object(i);
        
        MTL::LoadAction loadAction;

        switch (renderTargetParameters.LoadAction)
        {
            case ElemRenderPassLoadAction_Load:
                loadAction = MTL::LoadActionLoad;
                break;

            case ElemRenderPassLoadAction_Clear:
                loadAction = MTL::LoadActionClear;
                break;

            default:
                loadAction = MTL::LoadActionDontCare;
                break;
        }
        
        MTL::StoreAction storeAction;

        switch (renderTargetParameters.StoreAction)
        {
            case ElemRenderPassStoreAction_Store:
                storeAction = MTL::StoreActionStore;
                break;

            default:
                storeAction = MTL::StoreActionDontCare;
                break;
        }

        if (textureData->DeviceObject.get() == nullptr)
        {
            return;
        }

        renderTargetDescriptor->setTexture((MTL::Texture*)textureData->DeviceObject.get());
        renderTargetDescriptor->setLoadAction(loadAction);
        renderTargetDescriptor->setStoreAction(storeAction); 

        if (renderTargetParameters.LoadAction == ElemRenderPassLoadAction_Clear)
        {
            auto clearColor = renderTargetParameters.ClearColor;
            renderTargetDescriptor->setClearColor(MTL::ClearColor(clearColor.Red, clearColor.Green, clearColor.Blue, clearColor.Alpha));
        }
    } 
    
    auto renderCommandEncoder = NS::RetainPtr(commandListData->DeviceObject->renderCommandEncoder(renderPassDescriptor.get()));

    if (!renderCommandEncoder.get())
    {
        SystemLogErrorMessage(ElemLogMessageCategory_Graphics, "Render command encoder creation failed.");
        return;
    }
    
    commandListData->CommandEncoder = renderCommandEncoder;
    commandListData->CommandEncoderType = MetalCommandEncoderType_Render;
    commandListData->PipelineState = ELEM_HANDLE_NULL;
    
    //EnsureMetalResourceBarrier(commandList);

    if (parameters->Viewports.Length > 0)
    {
        ElemSetViewports(commandList, parameters->Viewports);
    }
    else if (parameters->RenderTargets.Length > 0)
    {
        auto renderTargetParameters = parameters->RenderTargets.Items[0];

        auto textureData = GetMetalResourceData(renderTargetParameters.RenderTarget); 
        SystemAssert(textureData);

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

void MetalEndRenderPass(ElemCommandList commandList)
{
    // TODO: Check command list type != COMPUTE

    ResetMetalCommandEncoder(commandList);
}

void MetalSetViewports(ElemCommandList commandList, ElemViewportSpan viewports)
{
    // TODO: Check command list type != COMPUTE

    SystemAssert(commandList != ELEM_HANDLE_NULL);

    auto commandListData = GetMetalCommandListData(commandList);
    SystemAssert(commandListData);

    SystemAssert(commandListData->CommandEncoderType == MetalCommandEncoderType_Render);
    SystemAssert(commandListData->CommandEncoder);

    auto renderCommandEncoder = (MTL::RenderCommandEncoder*)commandListData->CommandEncoder.get();

    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto metalViewports = SystemPushArray<MTL::Viewport>(stackMemoryArena, viewports.Length);
    auto metalScissorRects = SystemPushArray<MTL::ScissorRect>(stackMemoryArena, viewports.Length);

    for (uint32_t i = 0; i < viewports.Length; i++)
    {
        metalViewports[i] = 
        {
            .originX = viewports.Items[i].X,
            .originY = viewports.Items[i].Y,
            .width = viewports.Items[i].Width,
            .height = viewports.Items[i].Height,
            .znear = viewports.Items[i].MinDepth,
            .zfar = viewports.Items[i].MaxDepth
        };

        metalScissorRects[i] =
        {
            .x = (uint32_t)viewports.Items[i].X,
            .y = (uint32_t)viewports.Items[i].Y,
            .width = (uint32_t)viewports.Items[i].Width,
            .height = (uint32_t)viewports.Items[i].Height,
        };
    } 

    renderCommandEncoder->setViewports(metalViewports.Pointer, viewports.Length);
    renderCommandEncoder->setScissorRects(metalScissorRects.Pointer, viewports.Length);
}

void MetalDispatchMesh(ElemCommandList commandList, uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ)
{
    // TODO: Check command list type != COMPUTE
    // TODO: Check if render pass active

    SystemAssert(commandList != ELEM_HANDLE_NULL);

    auto commandListData = GetMetalCommandListData(commandList);
    SystemAssert(commandListData);

    SystemAssert(commandListData->CommandEncoderType == MetalCommandEncoderType_Render);
    SystemAssert(commandListData->CommandEncoder);

    auto pipelineStateData = GetMetalPipelineStateData(commandListData->PipelineState);
    SystemAssert(pipelineStateData);

    // TODO: Get the correct threads config for amplification shader
    auto renderCommandEncoder = (MTL::RenderCommandEncoder*)commandListData->CommandEncoder.get();
    renderCommandEncoder->drawMeshThreadgroups(MTL::Size(threadGroupCountX, threadGroupCountY, threadGroupCountZ), 
                                               MTL::Size(32, 1, 1), 
                                               MTL::Size(pipelineStateData->MeshShaderMetaData.ThreadSizeX, pipelineStateData->MeshShaderMetaData.ThreadSizeY, pipelineStateData->MeshShaderMetaData.ThreadSizeZ));
}
