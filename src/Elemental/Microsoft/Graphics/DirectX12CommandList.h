#pragma once

#include "Elemental.h"

struct DirectX12CommandQueueData
{
    ComPtr<ID3D12CommandQueue> DeviceObject;
};

struct DirectX12CommandQueueDataFull
{
    D3D12_COMMAND_LIST_TYPE Type;
};

DirectX12CommandQueueData* GetDirectX12CommandQueueData(ElemCommandQueue graphicsCommandQueue);
DirectX12CommandQueueDataFull* GetDirectX12CommandQueueDataFull(ElemCommandQueue graphicsCommandQueue);

ElemCommandQueue DirectX12CreateCommandQueue(ElemGraphicsDevice graphicsDevice, ElemCommandQueueType type, const ElemCommandQueueOptions* options);
void DirectX12FreeCommandQueue(ElemCommandQueue commandQueue);
ElemCommandList DirectX12CreateCommandList(ElemCommandQueue commandQueue, const ElemCommandListOptions* options);
void DirectX12CommitCommandList(ElemCommandList commandList);

ElemFence DirectX12ExecuteCommandList(ElemCommandQueue commandQueue, ElemCommandList commandList, const ElemExecuteCommandListOptions* options);
ElemFence DirectX12ExecuteCommandLists(ElemCommandQueue commandQueue, ElemCommandListSpan commandLists, const ElemExecuteCommandListOptions* options);