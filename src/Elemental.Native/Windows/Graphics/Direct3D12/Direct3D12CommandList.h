#pragma once

#include "GraphicsObject.h"
#include "Direct3D12Common.h"
#include "Direct3D12CommandQueue.h"

struct CommandAllocatorPoolItem;

struct Direct3D12CommandList : GraphicsObject
{
    Direct3D12CommandList(Direct3D12CommandQueue* commandQueue, Direct3D12GraphicsDevice* graphicsDevice)
    {
        GraphicsDevice = graphicsDevice;
        GraphicsApi = GraphicsApi_Direct3D12;
        CommandQueue = commandQueue;
        IsFromCommandPool = false;
        IsUsed = true;
        IsRenderPassActive = false;
        CommandAllocatorPoolItem = nullptr;
    }

    Direct3D12GraphicsDevice* GraphicsDevice;
    ComPtr<ID3D12GraphicsCommandList7> DeviceObject;
    Direct3D12CommandQueue* CommandQueue;
    RenderPassDescriptor CurrentRenderPassDescriptor;
    bool IsUsed;
    bool IsFromCommandPool;
    bool IsRenderPassActive;
    CommandAllocatorPoolItem* CommandAllocatorPoolItem;
};

