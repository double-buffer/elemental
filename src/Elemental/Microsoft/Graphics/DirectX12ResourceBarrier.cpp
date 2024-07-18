#include "DirectX12Resource.h"
#include "DirectX12GraphicsDevice.h"
#include "DirectX12CommandList.h"
#include "SystemFunctions.h"
#include "SystemMemory.h"

D3D12_BARRIER_SYNC ConvertToDirectX12BarrierSync(ResourceBarrierSyncType syncType)
{
    switch (syncType) 
    {
        case SyncType_None:
            return D3D12_BARRIER_SYNC_NONE;

        case SyncType_Compute:
            return D3D12_BARRIER_SYNC_COMPUTE_SHADING;
    }
}

D3D12_BARRIER_ACCESS ConvertToDirectX12BarrierAccess(ResourceBarrierAccessType accessType)
{
    // TODO: Recheck the correct accesses
    // Maybe we can pass more info to the function to compute more precides accesses (or in the common code)

    switch (accessType) 
    {
        case AccessType_NoAccess:
            return D3D12_BARRIER_ACCESS_NO_ACCESS;

        case AccessType_Read:
            return D3D12_BARRIER_ACCESS_COMMON;

        case AccessType_Write:
            return D3D12_BARRIER_ACCESS_UNORDERED_ACCESS;
    }
}

D3D12_BARRIER_LAYOUT ConvertToDirectX12BarrierLayout(ResourceBarrierLayoutType layoutType)
{
    // TODO: Recheck the correct layouts
    // Maybe we can pass more info to the function to compute more precides layouts (or in the common code)
    // It would be better for common layout to specialize that base on the current queue type???

    switch (layoutType) 
    {
        case LayoutType_Undefined:
            return D3D12_BARRIER_LAYOUT_UNDEFINED;

        case LayoutType_Read:
            return D3D12_BARRIER_LAYOUT_COMMON;

        case LayoutType_Write:
            return D3D12_BARRIER_LAYOUT_UNORDERED_ACCESS;

        case LayoutType_RenderTarget:
            return D3D12_BARRIER_LAYOUT_RENDER_TARGET;
    }
}

void InsertDirectX12ResourceBarriersIfNeeded(ElemCommandList commandList, ResourceBarrierSyncType currentStage)
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
            directX12BufferBarrier->SyncBefore = ConvertToDirectX12BarrierSync(barrier.SyncBefore);
            directX12BufferBarrier->SyncAfter = ConvertToDirectX12BarrierSync(barrier.SyncAfter);
            directX12BufferBarrier->AccessBefore = ConvertToDirectX12BarrierAccess(barrier.AccessBefore);
            directX12BufferBarrier->AccessAfter = ConvertToDirectX12BarrierAccess(barrier.AccessAfter);
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
            directX12TextureBarrier->SyncBefore = ConvertToDirectX12BarrierSync(barrier.SyncBefore);
            directX12TextureBarrier->SyncAfter = ConvertToDirectX12BarrierSync(barrier.SyncAfter);
            directX12TextureBarrier->AccessBefore = ConvertToDirectX12BarrierAccess(barrier.AccessBefore);
            directX12TextureBarrier->AccessAfter = ConvertToDirectX12BarrierAccess(barrier.AccessAfter);
            directX12TextureBarrier->LayoutBefore = ConvertToDirectX12BarrierLayout(barrier.LayoutBefore);
            directX12TextureBarrier->LayoutAfter = ConvertToDirectX12BarrierLayout(barrier.LayoutAfter);
        }
    }

    commandListData->DeviceObject->Barrier(barrierGroupCount, barrierGroups);
}

void DirectX12GraphicsResourceBarrier(ElemCommandList commandList, ElemGraphicsResourceDescriptor descriptor, const ElemGraphicsResourceBarrierOptions* options)
{
    SystemAssert(commandList != ELEM_HANDLE_NULL);

    auto commandListData = GetDirectX12CommandListData(commandList);
    SystemAssert(commandListData);

    auto descriptorInfo = DirectX12GetGraphicsResourceDescriptorInfo(descriptor);
    auto resourceInfo = DirectX12GetGraphicsResourceInfo(descriptorInfo.Resource);
    
    ResourceBarrierItem resourceBarrier =
    {
        .Type = resourceInfo.Type,
        .Resource = descriptorInfo.Resource,
        .AccessAfter = (descriptorInfo.Usage & ElemGraphicsResourceDescriptorUsage_Write) ? AccessType_Write : AccessType_Read,
        .LayoutAfter = (descriptorInfo.Usage & ElemGraphicsResourceDescriptorUsage_Write) ? LayoutType_Write : LayoutType_Read
        // TODO: Options
    };

    EnqueueBarrier(commandListData->ResourceBarrierPool, &resourceBarrier);
}
