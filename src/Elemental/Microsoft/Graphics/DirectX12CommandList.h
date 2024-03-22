#pragma once

#include "Elemental.h"

struct DirectX12CommandQueueData
{
    ComPtr<ID3D12CommandQueue> DeviceObject;
    D3D12_COMMAND_LIST_TYPE Type;
};

struct DirectX12CommandQueueDataFull
{
    ElemGraphicsDevice GraphicsDevice;
    ComPtr<ID3D12Fence1> Fence;
    uint64_t FenceValue = 0;
    uint64_t LastCompletedFenceValue = 0;

    // TODO: We use 2 command allocators for the moment. 
    // We will handle proper multi threading later
    ComPtr<ID3D12CommandAllocator> CommandAllocators[2];
};

struct DirectX12CommandListData
{
    ComPtr<ID3D12GraphicsCommandList7> DeviceObject;
};

struct DirectX12CommandListDataFull
{
    ElemBeginRenderPassOptions CurrentRenderPassOptions;
};

ElemFence Direct3D12CreateCommandQueueFence(ElemCommandQueue commandQueue);
DirectX12CommandQueueData* GetDirectX12CommandQueueData(ElemCommandQueue commandQueue);
DirectX12CommandQueueDataFull* GetDirectX12CommandQueueDataFull(ElemCommandQueue commandQueue);
DirectX12CommandListData* GetDirectX12CommandListData(ElemCommandList commandList);
DirectX12CommandListDataFull* GetDirectX12CommandListDataFull(ElemCommandList commandList);

ElemCommandQueue DirectX12CreateCommandQueue(ElemGraphicsDevice graphicsDevice, ElemCommandQueueType type, const ElemCommandQueueOptions* options);
void DirectX12FreeCommandQueue(ElemCommandQueue commandQueue);
ElemCommandList DirectX12CreateCommandList(ElemCommandQueue commandQueue, const ElemCommandListOptions* options);
void DirectX12CommitCommandList(ElemCommandList commandList);

ElemFence DirectX12ExecuteCommandLists(ElemCommandQueue commandQueue, ElemCommandListSpan commandLists, const ElemExecuteCommandListOptions* options);
void DirectX12WaitForFenceOnCpu(ElemFence fence);
