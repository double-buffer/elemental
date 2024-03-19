#pragma once

#include "Elemental.h"

struct Direct3D12CommandQueueData
{
    ComPtr<ID3D12CommandQueue> DeviceObject;
};

struct Direct3D12CommandQueueDataFull
{
    D3D12_COMMAND_LIST_TYPE Type;
};

Direct3D12CommandQueueData* GetDirect3D12CommandQueueData(ElemCommandQueue graphicsCommandQueue);
Direct3D12CommandQueueDataFull* GetDirect3D12CommandQueueDataFull(ElemCommandQueue graphicsCommandQueue);

ElemCommandQueue Direct3D12CreateCommandQueue(ElemGraphicsDevice graphicsDevice, ElemCommandQueueType type, const ElemCommandQueueOptions* options);
void Direct3D12FreeCommandQueue(ElemCommandQueue commandQueue);
ElemCommandList Direct3D12CreateCommandList(ElemCommandQueue commandQueue, const ElemCommandListOptions* options);
void Direct3D12CommitCommandList(ElemCommandList commandList);

ElemFence Direct3D12ExecuteCommandList(ElemCommandQueue commandQueue, ElemCommandList commandList, const ElemExecuteCommandListOptions* options);
ElemFence Direct3D12ExecuteCommandLists(ElemCommandQueue commandQueue, ElemCommandListSpan commandLists, const ElemExecuteCommandListOptions* options);
