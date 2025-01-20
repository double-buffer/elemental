#include "DirectX12Resource.h"
#include "DirectX12GraphicsDevice.h"
#include "DirectX12CommandList.h"
#include "SystemFunctions.h"
#include "SystemMemory.h"

D3D12_BARRIER_SYNC ConvertToDirectX12BarrierSync(ElemGraphicsResourceBarrierSyncType syncType, bool isDepthStencil)
{
    switch (syncType) 
    {
        case ElemGraphicsResourceBarrierSyncType_None:
            return D3D12_BARRIER_SYNC_NONE;

        case ElemGraphicsResourceBarrierSyncType_Compute:
            return D3D12_BARRIER_SYNC_COMPUTE_SHADING;

        case ElemGraphicsResourceBarrierSyncType_Copy:
            return D3D12_BARRIER_SYNC_COPY;

        case ElemGraphicsResourceBarrierSyncType_BuildRaytracingAccelerationStructure:
            return D3D12_BARRIER_SYNC_BUILD_RAYTRACING_ACCELERATION_STRUCTURE;

        case ElemGraphicsResourceBarrierSyncType_RenderTarget:
            return !isDepthStencil ? D3D12_BARRIER_SYNC_RENDER_TARGET : D3D12_BARRIER_SYNC_DEPTH_STENCIL;
    }
}

D3D12_BARRIER_ACCESS ConvertToDirectX12BarrierAccess(ElemGraphicsResourceBarrierAccessType accessType, bool isAccelerationStructure)
{
    // TODO: Recheck the correct accesses
    // Maybe we can pass more info to the function to compute more precides accesses (or in the common code)

    switch (accessType) 
    {
        case ElemGraphicsResourceBarrierAccessType_NoAccess:
            return D3D12_BARRIER_ACCESS_NO_ACCESS;

        case ElemGraphicsResourceBarrierAccessType_Read:
            return isAccelerationStructure ? D3D12_BARRIER_ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_READ : D3D12_BARRIER_ACCESS_COMMON;

        case ElemGraphicsResourceBarrierAccessType_RenderTarget:
            return D3D12_BARRIER_ACCESS_RENDER_TARGET;

        case ElemGraphicsResourceBarrierAccessType_DepthStencilWrite:
            return D3D12_BARRIER_ACCESS_DEPTH_STENCIL_WRITE;

        case ElemGraphicsResourceBarrierAccessType_Write:
            return D3D12_BARRIER_ACCESS_UNORDERED_ACCESS;

        case ElemGraphicsResourceBarrierAccessType_Copy:
            return D3D12_BARRIER_ACCESS_COPY_DEST;
    }
}

D3D12_BARRIER_LAYOUT ConvertToDirectX12BarrierLayout(ElemGraphicsResourceBarrierLayoutType layoutType)
{
    // TODO: Recheck the correct layouts
    // Maybe we can pass more info to the function to compute more precides layouts (or in the common code)
    // It would be better for common layout to specialize that base on the current queue type???

    switch (layoutType) 
    {
        case ElemGraphicsResourceBarrierLayoutType_Undefined:
            return D3D12_BARRIER_LAYOUT_UNDEFINED;

        case ElemGraphicsResourceBarrierLayoutType_Read:
            return D3D12_BARRIER_LAYOUT_COMMON;

        case ElemGraphicsResourceBarrierLayoutType_Write:
            return D3D12_BARRIER_LAYOUT_UNORDERED_ACCESS;

        case ElemGraphicsResourceBarrierLayoutType_RenderTarget:
            return D3D12_BARRIER_LAYOUT_RENDER_TARGET;

        case ElemGraphicsResourceBarrierLayoutType_DepthStencilWrite:
            return D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_WRITE;

        case ElemGraphicsResourceBarrierLayoutType_Present:
            return D3D12_BARRIER_LAYOUT_PRESENT;
    }
}

void InsertDirectX12ResourceBarriersIfNeeded(ElemCommandList commandList, ElemGraphicsResourceBarrierSyncType currentStage)
{
    auto stackMemoryArena = SystemGetStackMemoryArena();
    
    SystemAssert(commandList != ELEM_HANDLE_NULL);

    auto commandListData = GetDirectX12CommandListData(commandList);
    SystemAssert(commandListData);
    
    auto barriersInfo = GenerateBarrierCommands(stackMemoryArena, commandListData->ResourceBarrierPool, currentStage, DirectX12DebugBarrierInfoEnabled);

    if (barriersInfo.BufferBarriers.Length == 0 && barriersInfo.TextureBarriers.Length == 0)
    {
        return;
    }

    D3D12_BARRIER_GROUP barrierGroups[2];
    uint32_t barrierGroupCount = 0;

    if (barriersInfo.BufferBarriers.Length > 0)
    {
        auto directX12BufferBarriers = SystemPushArray<D3D12_BUFFER_BARRIER>(stackMemoryArena, barriersInfo.BufferBarriers.Length);

        D3D12_BARRIER_GROUP directX12BufferBarriersGroup =
        {
            .Type = D3D12_BARRIER_TYPE_BUFFER,
            .NumBarriers = (uint32_t)barriersInfo.BufferBarriers.Length,
            .pBufferBarriers = directX12BufferBarriers.Pointer
        };

        barrierGroups[barrierGroupCount++] = directX12BufferBarriersGroup;

        for (uint32_t i = 0; i < barriersInfo.BufferBarriers.Length; i++)
        {
            auto barrier = barriersInfo.BufferBarriers[i];
            auto directX12BufferBarrier = &directX12BufferBarriers[i];

            auto graphicsResourceData = GetDirectX12GraphicsResourceData(barrier.Resource);
            SystemAssert(graphicsResourceData);

            directX12BufferBarrier->pResource = graphicsResourceData->DeviceObject.Get();
            directX12BufferBarrier->Size = graphicsResourceData->Width;
            directX12BufferBarrier->SyncBefore = ConvertToDirectX12BarrierSync(barrier.BeforeSync, false);
            directX12BufferBarrier->SyncAfter = ConvertToDirectX12BarrierSync(barrier.AfterSync, false);
            directX12BufferBarrier->AccessBefore = ConvertToDirectX12BarrierAccess(barrier.BeforeAccess, graphicsResourceData->Type == ElemGraphicsResourceType_RaytracingAccelerationStructure);
            directX12BufferBarrier->AccessAfter = ConvertToDirectX12BarrierAccess(barrier.AfterAccess, graphicsResourceData->Type == ElemGraphicsResourceType_RaytracingAccelerationStructure);
        }
    }

    if (barriersInfo.TextureBarriers.Length > 0)
    {
        auto directX12TextureBarriers = SystemPushArray<D3D12_TEXTURE_BARRIER>(stackMemoryArena, barriersInfo.TextureBarriers.Length);

        D3D12_BARRIER_GROUP directX12TextureBarriersGroup =
        {
            .Type = D3D12_BARRIER_TYPE_TEXTURE,
            .NumBarriers = (uint32_t)barriersInfo.TextureBarriers.Length,
            .pTextureBarriers = directX12TextureBarriers.Pointer
        };

        barrierGroups[barrierGroupCount++] = directX12TextureBarriersGroup;

        for (uint32_t i = 0; i < barriersInfo.TextureBarriers.Length; i++)
        {
            auto barrier = barriersInfo.TextureBarriers[i];
            auto directX12TextureBarrier = &directX12TextureBarriers[i];

            auto graphicsResourceData = GetDirectX12GraphicsResourceData(barrier.Resource);
            SystemAssert(graphicsResourceData);

            directX12TextureBarrier->pResource = graphicsResourceData->DeviceObject.Get();
            directX12TextureBarrier->SyncBefore = ConvertToDirectX12BarrierSync(barrier.BeforeSync, barrier.IsDepthStencil);
            directX12TextureBarrier->SyncAfter = ConvertToDirectX12BarrierSync(barrier.AfterSync, barrier.IsDepthStencil);
            directX12TextureBarrier->AccessBefore = ConvertToDirectX12BarrierAccess(barrier.BeforeAccess, false);
            directX12TextureBarrier->AccessAfter = ConvertToDirectX12BarrierAccess(barrier.AfterAccess, false);
            directX12TextureBarrier->LayoutBefore = ConvertToDirectX12BarrierLayout(barrier.BeforeLayout);
            directX12TextureBarrier->LayoutAfter = ConvertToDirectX12BarrierLayout(barrier.AfterLayout);
        }
    }

    commandListData->DeviceObject->Barrier(barrierGroupCount, barrierGroups);
}

void DirectX12GraphicsResourceBarrier(ElemCommandList commandList, ElemGraphicsResourceDescriptor descriptor, const ElemGraphicsResourceBarrierOptions* options)
{
    SystemAssert(commandList != ELEM_HANDLE_NULL);

    auto commandListData = GetDirectX12CommandListData(commandList);
    SystemAssert(commandListData);

    EnqueueBarrier(commandListData->ResourceBarrierPool, descriptor, options);
}
