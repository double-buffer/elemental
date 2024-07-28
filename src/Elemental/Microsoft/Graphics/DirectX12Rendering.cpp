#include "DirectX12Rendering.h"
#include "DirectX12ResourceBarrier.h"
#include "DirectX12GraphicsDevice.h"
#include "DirectX12CommandList.h"
#include "DirectX12Resource.h"
#include "SystemFunctions.h"

void DirectX12BeginRenderPass(ElemCommandList commandList, const ElemBeginRenderPassParameters* parameters)
{
    // TODO: Check command list type != COMPUTE
    
    auto stackMemoryArena = SystemGetStackMemoryArena();

    SystemAssert(commandList != ELEM_HANDLE_NULL);
    SystemAssert(parameters);

    auto commandListData = GetDirectX12CommandListData(commandList);
    SystemAssert(commandListData);

    auto commandListDataFull = GetDirectX12CommandListDataFull(commandList);
    SystemAssert(commandListDataFull);
    commandListDataFull->CurrentRenderPassParameters = *parameters;

    auto graphicsDeviceData = GetDirectX12GraphicsDeviceData(commandListData->GraphicsDevice);
    SystemAssert(graphicsDeviceData);

    auto renderTargetDescList = SystemPushArray<D3D12_RENDER_PASS_RENDER_TARGET_DESC>(stackMemoryArena, parameters->RenderTargets.Length);

    for (uint32_t i = 0; i < parameters->RenderTargets.Length; i++)
    {
        auto renderTargetParameters = parameters->RenderTargets.Items[i];
        SystemAssert(renderTargetParameters.RenderTarget != ELEM_HANDLE_NULL);

        auto textureData = GetDirectX12GraphicsResourceData(renderTargetParameters.RenderTarget);
        SystemAssert(textureData);

        // TODO: Validate usage

        D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE beginAccessType;

        switch (renderTargetParameters.LoadAction)
        {
            case ElemRenderPassLoadAction_Load:
                beginAccessType = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE;
                break;

            case ElemRenderPassLoadAction_Clear:
                beginAccessType = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
                break;

            default:
                beginAccessType = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD;
                break;
        }
        
        D3D12_RENDER_PASS_BEGINNING_ACCESS_CLEAR_PARAMETERS beginAccessClearValue
        {
            .ClearValue = 
            { 
                .Format = textureData->DirectX12Format, 
                .Color = 
                { 
                    renderTargetParameters.ClearColor.Red, 
                    renderTargetParameters.ClearColor.Green, 
                    renderTargetParameters.ClearColor.Blue, 
                    renderTargetParameters.ClearColor.Alpha
                }
            }
        };

        D3D12_RENDER_PASS_ENDING_ACCESS_TYPE endAccessType;

        switch (renderTargetParameters.StoreAction)
        {
            case ElemRenderPassStoreAction_Store:
                endAccessType = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
                break;

            default:
                endAccessType = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_DISCARD;
                break;
        }

        renderTargetDescList[i] =
        {
            .cpuDescriptor = textureData->RtvHandle,
            .BeginningAccess = { .Type = beginAccessType, .Clear = beginAccessClearValue },
            .EndingAccess = { .Type = endAccessType }
        };

        ResourceBarrierItem resourceBarrier =
        {
            .Type = ElemGraphicsResourceType_Texture2D,
            .Resource = renderTargetParameters.RenderTarget,
            .AfterAccess = ElemGraphicsResourceBarrierAccessType_RenderTarget,
            .AfterLayout = ElemGraphicsResourceBarrierLayoutType_RenderTarget
        };

        EnqueueBarrier(commandListData->ResourceBarrierPool, &resourceBarrier);
        
        if (i == 0 && parameters->Viewports.Length == 0)
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

    if (parameters->Viewports.Length > 0)
    {
        ElemSetViewports(commandList, parameters->Viewports);
    }

    InsertDirectX12ResourceBarriersIfNeeded(commandList, ElemGraphicsResourceBarrierSyncType_RenderTarget);
    commandListData->DeviceObject->BeginRenderPass(renderTargetDescList.Length, renderTargetDescList.Pointer, nullptr, D3D12_RENDER_PASS_FLAG_NONE);
}

void DirectX12EndRenderPass(ElemCommandList commandList)
{
    // TODO: Check command list type != COMPUTE

    SystemAssert(commandList != ELEM_HANDLE_NULL);

    auto commandListData = GetDirectX12CommandListData(commandList);
    SystemAssert(commandListData);

    auto commandListDataFull = GetDirectX12CommandListDataFull(commandList);
    SystemAssert(commandListDataFull);
    auto parameters = &commandListDataFull->CurrentRenderPassParameters;

    commandListData->DeviceObject->EndRenderPass();

    for (uint32_t i = 0; i < parameters->RenderTargets.Length; i++)
    {
        auto renderTargetParameters = parameters->RenderTargets.Items[i];
        SystemAssert(renderTargetParameters.RenderTarget != ELEM_HANDLE_NULL);

        auto textureData = GetDirectX12GraphicsResourceData(renderTargetParameters.RenderTarget); 
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
        }
    }
    
    InsertDirectX12ResourceBarriersIfNeeded(commandList, ElemGraphicsResourceBarrierSyncType_None);
}

void DirectX12SetViewports(ElemCommandList commandList, ElemViewportSpan viewports)
{
    // TODO: Check command list type != COMPUTE

    SystemAssert(commandList != ELEM_HANDLE_NULL);

    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto directX12Viewports = SystemPushArray<D3D12_VIEWPORT>(stackMemoryArena, viewports.Length);
    auto directX12ScissorRects = SystemPushArray<D3D12_RECT>(stackMemoryArena, viewports.Length);

    for (uint32_t i = 0; i < viewports.Length; i++)
    {
        directX12Viewports[i] = 
        {
            .TopLeftX = viewports.Items[i].X,
            .TopLeftY = viewports.Items[i].Y,
            .Width = viewports.Items[i].Width,
            .Height = viewports.Items[i].Height,
            .MinDepth = viewports.Items[i].MinDepth,
            .MaxDepth = viewports.Items[i].MaxDepth
        };

        directX12ScissorRects[i] = 
        {
            .left = (int32_t)viewports.Items[i].X,
            .top = (int32_t)viewports.Items[i].Y,
            .right = (int32_t)viewports.Items[i].X + (int32_t)viewports.Items[i].Width,
            .bottom = (int32_t)viewports.Items[i].Y + (int32_t)viewports.Items[i].Height
        };
    }

    auto commandListData = GetDirectX12CommandListData(commandList);
    SystemAssert(commandListData);

    commandListData->DeviceObject->RSSetViewports(directX12Viewports.Length, directX12Viewports.Pointer);
    commandListData->DeviceObject->RSSetScissorRects(directX12ScissorRects.Length, directX12ScissorRects.Pointer);
}

void DirectX12DispatchMesh(ElemCommandList commandList, uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ)
{
    // TODO: Check command list type != COMPUTE
    // TODO: Check if render pass active

    SystemAssert(commandList != ELEM_HANDLE_NULL);
    SystemAssert(threadGroupCountX > 0);
    SystemAssert(threadGroupCountY > 0);
    SystemAssert(threadGroupCountZ > 0);

    auto commandListData = GetDirectX12CommandListData(commandList);
    SystemAssert(commandListData);

    commandListData->DeviceObject->DispatchMesh(threadGroupCountX, threadGroupCountY, threadGroupCountZ);
}
