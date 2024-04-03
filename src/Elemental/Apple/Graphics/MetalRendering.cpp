#include "MetalRendering.h"
#include "MetalCommandList.h"
#include "MetalTexture.h"
#include "SystemFunctions.h"
#include "SystemLogging.h"

void MetalBeginRenderPass(ElemCommandList commandList, const ElemBeginRenderPassParameters* parameters)
{
    auto stackMemoryArena = SystemGetStackMemoryArena();

    SystemAssert(commandList != ELEM_HANDLE_NULL);
    SystemAssert(parameters);

    auto commandListData = GetMetalCommandListData(commandList);
    SystemAssert(commandListData);

    auto commandListDataFull = GetMetalCommandListDataFull(commandList);
    SystemAssert(commandListDataFull);

    auto renderPassDescriptor = NS::TransferPtr(MTL::RenderPassDescriptor::alloc()->init());

    for (uint32_t i = 0; i < parameters->RenderTargets.Length; i++)
    {
        auto renderTargetParameters = parameters->RenderTargets.Items[i];
        SystemAssert(renderTargetParameters.RenderTarget != ELEM_HANDLE_NULL);

        auto textureData = GetMetalTextureData(renderTargetParameters.RenderTarget); 
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

        renderTargetDescriptor->setTexture(textureData->DeviceObject.get());
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

    if (parameters->RenderTargets.Length > 0)
    {
        auto renderTargetParameters = parameters->RenderTargets.Items[0];
        SystemAssert(renderTargetParameters.RenderTarget != ELEM_HANDLE_NULL);

        auto textureData = GetMetalTextureData(renderTargetParameters.RenderTarget); 
        SystemAssert(textureData);

        MTL::Viewport viewport = 
        {
            .width = (float)textureData->Width, 
            .height = (float)textureData->Height, 
            .znear = 0.0f,
            .zfar = 1.0f
        };
        
        renderCommandEncoder->setViewport(viewport);

        MTL::ScissorRect scissorRect = 
        {
            .width = textureData->Width,
            .height = textureData->Height
        };

        renderCommandEncoder->setScissorRect(scissorRect);
    }

    commandListData->CommandEncoder = renderCommandEncoder;
}

void MetalEndRenderPass(ElemCommandList commandList)
{
    ResetMetalCommandEncoder(commandList);
}

void MetalDispatchMesh(ElemCommandList commandList, uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ)
{
}
