#pragma once

#include "Elemental.h"

struct Direct3D12GraphicsCommandQueueData
{
    ComPtr<ID3D12CommandQueue> DeviceObject;
};

struct Direct3D12GraphicsCommandQueueDataFull
{
    D3D12_COMMAND_LIST_TYPE Type;
};

Direct3D12GraphicsCommandQueueData* GetDirect3D12GraphicsCommandQueueData(ElemGraphicsCommandQueue graphicsCommandQueue);
Direct3D12GraphicsCommandQueueDataFull* GetDirect3D12GraphicsCommandQueueDataFull(ElemGraphicsCommandQueue graphicsCommandQueue);

ElemGraphicsCommandQueue Direct3D12CreateGraphicsCommandQueue(ElemGraphicsDevice graphicsDevice, ElemGraphicsCommandQueueType type, const ElemGraphicsCommandQueueOptions* options);
void Direct3D12FreeGraphicsCommandQueue(ElemGraphicsCommandQueue commandQueue);
