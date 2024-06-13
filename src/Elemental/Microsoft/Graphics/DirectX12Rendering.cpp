#include "DirectX12Rendering.h"
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

    auto renderTargetDescList = SystemPushArray<D3D12_RENDER_PASS_RENDER_TARGET_DESC>(stackMemoryArena, parameters->RenderTargets.Length);

    for (uint32_t i = 0; i < parameters->RenderTargets.Length; i++)
    {
        auto renderTargetParameters = parameters->RenderTargets.Items[i];
        SystemAssert(renderTargetParameters.RenderTarget != ELEM_HANDLE_NULL);

        auto textureData = GetDirectX12TextureData(renderTargetParameters.RenderTarget); 
        SystemAssert(textureData);

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
                .Format = textureData->ResourceDescription.Format, 
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

        // TODO: Group the barriers with a barrier system
        renderTargetDescList[i] =
        {
            .cpuDescriptor = textureData->RtvDescriptor,
            .BeginningAccess = { .Type = beginAccessType, .Clear = beginAccessClearValue },
            .EndingAccess = { .Type = endAccessType }
        };

        //⚠️ : All barrier stuff will have a common logic and will try to maximize the grouping of barriers!!!
        // See: https://github.com/TheRealMJP/MemPoolTest/blob/8d7a5b9af5e6f1fe4ff3a35ba51aeb7924183ae2/SampleFramework12/v1.04/Graphics/GraphicsTypes.h#L461
        if (textureData->IsPresentTexture)
        {
            D3D12_TEXTURE_BARRIER renderTargetBarrier = {};
            renderTargetBarrier.AccessBefore = D3D12_BARRIER_ACCESS_NO_ACCESS;
            renderTargetBarrier.LayoutBefore = D3D12_BARRIER_LAYOUT_UNDEFINED;
            renderTargetBarrier.SyncBefore = D3D12_BARRIER_SYNC_NONE;
            renderTargetBarrier.AccessAfter = D3D12_BARRIER_ACCESS_RENDER_TARGET;
            renderTargetBarrier.LayoutAfter = D3D12_BARRIER_LAYOUT_RENDER_TARGET;
            renderTargetBarrier.SyncAfter = D3D12_BARRIER_SYNC_ALL; // TODO: Should use sync render target?
            renderTargetBarrier.pResource = textureData->DeviceObject.Get();

            D3D12_BARRIER_GROUP textureBarriersGroup;
            textureBarriersGroup = {};
            textureBarriersGroup.Type = D3D12_BARRIER_TYPE_TEXTURE;
            textureBarriersGroup.NumBarriers = 1;
            textureBarriersGroup.pTextureBarriers = &renderTargetBarrier;

            commandListData->DeviceObject->Barrier(1, &textureBarriersGroup);
        }
        
        // TODO: To remove
        //commandListData->DeviceObject->OMSetRenderTargets(1, &textureData->RtvDescriptor, FALSE, nullptr);
        //const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
        //commandListData->DeviceObject->ClearRenderTargetView(textureData->RtvDescriptor, clearColor, 0, nullptr);

        if (i == 0 && parameters->Viewports.Length == 0)
        {
            ElemViewport viewport =
            {
                .X = 0, 
                .Y = 0, 
                .Width = (float)textureData->ResourceDescription.Width, 
                .Height = (float)textureData->ResourceDescription.Height, 
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

        auto textureData = GetDirectX12TextureData(renderTargetParameters.RenderTarget); 
        SystemAssert(textureData);

        //⚠️ : All barrier stuff will have a common logic and will try to maximize the grouping of barriers!!!
        if (textureData->IsPresentTexture)
        {
            D3D12_TEXTURE_BARRIER renderTargetBarrier = {};
            renderTargetBarrier.AccessBefore = D3D12_BARRIER_ACCESS_RENDER_TARGET;
            renderTargetBarrier.LayoutBefore = D3D12_BARRIER_LAYOUT_RENDER_TARGET;
            renderTargetBarrier.SyncBefore = D3D12_BARRIER_SYNC_RENDER_TARGET;
            renderTargetBarrier.AccessAfter = D3D12_BARRIER_ACCESS_NO_ACCESS;
            renderTargetBarrier.LayoutAfter = D3D12_BARRIER_LAYOUT_PRESENT;
            renderTargetBarrier.SyncAfter = D3D12_BARRIER_SYNC_NONE;
            renderTargetBarrier.pResource = textureData->DeviceObject.Get();

            D3D12_BARRIER_GROUP textureBarriersGroup;
            textureBarriersGroup = {};
            textureBarriersGroup.Type = D3D12_BARRIER_TYPE_TEXTURE;
            textureBarriersGroup.NumBarriers = 1;
            textureBarriersGroup.pTextureBarriers = &renderTargetBarrier;

            commandListData->DeviceObject->Barrier(1, &textureBarriersGroup);
        }
    }
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
