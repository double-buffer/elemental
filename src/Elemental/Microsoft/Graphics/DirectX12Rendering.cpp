#include "DirectX12GraphicsDevice.h"
#include "DirectX12CommandList.h"
#include "DirectX12Texture.h"
#include "SystemLogging.h"
#include "SystemFunctions.h"

void DirectX12BeginRenderPass(ElemCommandList commandList, const ElemBeginRenderPassOptions* options)
{
    auto stackMemoryArena = SystemGetStackMemoryArena();

    SystemAssert(commandList != ELEM_HANDLE_NULL);
    SystemAssert(options);

    auto commandListData = GetDirectX12CommandListData(commandList);
    SystemAssert(commandListData);

    auto commandListDataFull = GetDirectX12CommandListDataFull(commandList);
    SystemAssert(commandListDataFull);
    commandListDataFull->CurrentRenderPassOptions = *options;

    auto renderTargetDescList = SystemPushArray<D3D12_RENDER_PASS_RENDER_TARGET_DESC>(stackMemoryArena, options->RenderTargets.Length);

    for (uint32_t i = 0; i < options->RenderTargets.Length; i++)
    {
        auto renderTargetOptions = options->RenderTargets.Items[i];
        SystemAssert(renderTargetOptions.RenderTarget != ELEM_HANDLE_NULL);

        auto textureData = GetDirectX12TextureData(renderTargetOptions.RenderTarget); 
        SystemAssert(textureData);

        D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE beginAccessType;

        switch (renderTargetOptions.LoadAction)
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
                    renderTargetOptions.ClearColor.Red, 
                    renderTargetOptions.ClearColor.Green, 
                    renderTargetOptions.ClearColor.Blue, 
                    renderTargetOptions.ClearColor.Alpha
                }
            }
        };

        D3D12_RENDER_PASS_ENDING_ACCESS_TYPE endAccessType;

        switch (renderTargetOptions.StoreAction)
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
        if (textureData->IsPresentTexture)
        {
            D3D12_TEXTURE_BARRIER renderTargetBarrier = {};
            renderTargetBarrier.AccessBefore = D3D12_BARRIER_ACCESS_NO_ACCESS;
            renderTargetBarrier.AccessAfter = D3D12_BARRIER_ACCESS_RENDER_TARGET;
            renderTargetBarrier.LayoutBefore = D3D12_BARRIER_LAYOUT_COMMON;
            renderTargetBarrier.LayoutAfter = D3D12_BARRIER_LAYOUT_RENDER_TARGET;
            renderTargetBarrier.SyncBefore = D3D12_BARRIER_SYNC_NONE;
            renderTargetBarrier.SyncAfter = D3D12_BARRIER_SYNC_ALL;
            renderTargetBarrier.pResource = textureData->DeviceObject.Get();

            D3D12_BARRIER_GROUP textureBarriersGroup;
            textureBarriersGroup = {};
            textureBarriersGroup.Type = D3D12_BARRIER_TYPE_TEXTURE;
            textureBarriersGroup.NumBarriers = 1;
            textureBarriersGroup.pTextureBarriers = &renderTargetBarrier;

            commandListData->DeviceObject->Barrier(1, &textureBarriersGroup);
        }

        if (i == 0)
        {
            D3D12_VIEWPORT viewport = {};
            viewport.Width = (float)textureData->ResourceDescription.Width;
            viewport.Height = (float)textureData->ResourceDescription.Height;
            viewport.MinDepth = 0.0f;
            viewport.MaxDepth = 1.0f;
            commandListData->DeviceObject->RSSetViewports(1, &viewport);

            D3D12_RECT scissorRect = {};
            scissorRect.right = (long)textureData->ResourceDescription.Width;
            scissorRect.bottom = (long)textureData->ResourceDescription.Height;
            commandListData->DeviceObject->RSSetScissorRects(1, &scissorRect);
        }
    }  
    
    commandListData->DeviceObject->BeginRenderPass(renderTargetDescList.Length, renderTargetDescList.Pointer, nullptr, D3D12_RENDER_PASS_FLAG_NONE);
}

void DirectX12EndRenderPass(ElemCommandList commandList)
{
    SystemAssert(commandList != ELEM_HANDLE_NULL);

    auto commandListData = GetDirectX12CommandListData(commandList);
    SystemAssert(commandListData);

    auto commandListDataFull = GetDirectX12CommandListDataFull(commandList);
    SystemAssert(commandListDataFull);
    auto options = &commandListDataFull->CurrentRenderPassOptions;

    commandListData->DeviceObject->EndRenderPass();

    for (uint32_t i = 0; i < options->RenderTargets.Length; i++)
    {
        auto renderTargetOptions = options->RenderTargets.Items[i];
        SystemAssert(renderTargetOptions.RenderTarget != ELEM_HANDLE_NULL);

        auto textureData = GetDirectX12TextureData(renderTargetOptions.RenderTarget); 
        SystemAssert(textureData);

        //⚠️ : All barrier stuff will have a common logic and will try to maximize the grouping of barriers!!!
        if (textureData->IsPresentTexture)
        {
            D3D12_TEXTURE_BARRIER renderTargetBarrier = {};
            renderTargetBarrier.AccessBefore = D3D12_BARRIER_ACCESS_RENDER_TARGET;
            renderTargetBarrier.AccessAfter = D3D12_BARRIER_ACCESS_COMMON;
            renderTargetBarrier.LayoutBefore = D3D12_BARRIER_LAYOUT_RENDER_TARGET;
            renderTargetBarrier.LayoutAfter = D3D12_BARRIER_LAYOUT_PRESENT;
            renderTargetBarrier.SyncBefore = D3D12_BARRIER_SYNC_RENDER_TARGET;
            renderTargetBarrier.SyncAfter = D3D12_BARRIER_SYNC_ALL;
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
