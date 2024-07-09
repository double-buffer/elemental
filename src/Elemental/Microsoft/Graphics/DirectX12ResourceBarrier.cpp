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

void InsertDirectX12ResourceBarriersIfNeeded(ElemCommandList commandList, ResourceBarrierSyncType currentStage)
{
    auto stackMemoryArena = SystemGetStackMemoryArena();
    
    SystemAssert(commandList != ELEM_HANDLE_NULL);

    auto commandListData = GetDirectX12CommandListData(commandList);
    SystemAssert(commandListData);
    
    auto barriersInfo = GenerateBarrierCommands(stackMemoryArena, commandListData->ResourceBarrierPool, currentStage, DirectX12DebugBarrierInfoEnabled);

    auto directX12BufferBarriers = SystemPushArray<D3D12_BUFFER_BARRIER>(stackMemoryArena, barriersInfo.BufferBarriers.Length);

    D3D12_BARRIER_GROUP directX12BufferBarriersGroup =
    {
        .Type = D3D12_BARRIER_TYPE_BUFFER,
        .NumBarriers = (uint32_t)barriersInfo.BufferBarriers.Length,
        .pBufferBarriers = directX12BufferBarriers.Pointer
    };

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

    // TODO: Texture barriers
    /*
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
    textureBarriersGroup.pTextureBarriers = &renderTargetBarrier;*/

    commandListData->DeviceObject->Barrier(1, &directX12BufferBarriersGroup);
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
        .AccessAfter = (descriptorInfo.Usage & ElemGraphicsResourceDescriptorUsage_Write) ? AccessType_Write : AccessType_Read
        // TODO: Options
    };

    EnqueueBarrier(commandListData->ResourceBarrierPool, &resourceBarrier);
}
