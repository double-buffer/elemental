#pragma once

#include "Elemental.h"
#include "SystemSpan.h"
#include "Graphics/CommandAllocatorPool.h"

enum DirectX12PipelineStateType
{
    DirectX12PipelineStateType_Graphics,
    DirectX12PipelineStateType_Compute
};

struct DirectX12CommandQueueData
{
    ComPtr<ID3D12CommandQueue> DeviceObject;
    D3D12_COMMAND_LIST_TYPE Type;
    CommandAllocatorQueueType CommandAllocatorQueueType;
    ElemGraphicsDevice GraphicsDevice;
};

struct DirectX12CommandQueueDataFull
{
    ComPtr<ID3D12Fence1> Fence;
    uint64_t FenceValue = 0;
    uint64_t LastCompletedFenceValue = 0;
    Span<ComPtr<ID3D12CommandAllocator>> CommandAllocators;
    uint32_t CurrentCommandAllocatorIndex;
    Span<ComPtr<ID3D12GraphicsCommandList10>> CommandLists;
    uint32_t CurrentCommandListIndex;
};

struct DirectX12CommandListData
{
    ID3D12GraphicsCommandList10* DeviceObject;
    CommandAllocatorPoolItem<ID3D12CommandAllocator*, ID3D12GraphicsCommandList10*>* CommandAllocatorPoolItem;
    CommandListPoolItem<ID3D12GraphicsCommandList10*>* CommandListPoolItem;
    DirectX12PipelineStateType PipelineStateType;
    ElemGraphicsDevice GraphicsDevice;
    bool IsCommitted;
};

struct DirectX12CommandListDataFull
{
    ElemBeginRenderPassParameters CurrentRenderPassParameters;
};

ElemFence CreateDirectX12CommandQueueFence(ElemCommandQueue commandQueue);
DirectX12CommandQueueData* GetDirectX12CommandQueueData(ElemCommandQueue commandQueue);
DirectX12CommandQueueDataFull* GetDirectX12CommandQueueDataFull(ElemCommandQueue commandQueue);
DirectX12CommandListData* GetDirectX12CommandListData(ElemCommandList commandList);
DirectX12CommandListDataFull* GetDirectX12CommandListDataFull(ElemCommandList commandList);

ElemCommandQueue DirectX12CreateCommandQueue(ElemGraphicsDevice graphicsDevice, ElemCommandQueueType type, const ElemCommandQueueOptions* options);
void DirectX12FreeCommandQueue(ElemCommandQueue commandQueue);
void DirectX12ResetCommandAllocation(ElemGraphicsDevice graphicsDevice);
ElemCommandList DirectX12GetCommandList(ElemCommandQueue commandQueue, const ElemCommandListOptions* options);
void DirectX12CommitCommandList(ElemCommandList commandList);

ElemFence DirectX12ExecuteCommandLists(ElemCommandQueue commandQueue, ElemCommandListSpan commandLists, const ElemExecuteCommandListOptions* options);
void DirectX12WaitForFenceOnCpu(ElemFence fence);
bool DirectX12IsFenceCompleted(ElemFence fence);
