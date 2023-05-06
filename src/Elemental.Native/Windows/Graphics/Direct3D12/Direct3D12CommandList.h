#pragma once

struct CommandAllocatorPoolItem;

struct Direct3D12CommandList : Direct3D12BaseGraphicsObject
{
    Direct3D12CommandList(Direct3D12CommandQueue* commandQueue, BaseGraphicsService* graphicsService, Direct3D12GraphicsDevice* graphicsDevice) : Direct3D12BaseGraphicsObject(graphicsService, graphicsDevice)
    {
        CommandQueue = commandQueue;
        IsFromCommandPool = false;
        IsUsed = true;
        IsRenderPassActive = false;
        CommandAllocatorPoolItem = nullptr;
    }

    ComPtr<ID3D12GraphicsCommandList7> DeviceObject;
    Direct3D12CommandQueue* CommandQueue;
    RenderPassDescriptor CurrentRenderPassDescriptor;
    bool IsUsed;
    bool IsFromCommandPool;
    bool IsRenderPassActive;
    CommandAllocatorPoolItem* CommandAllocatorPoolItem;
};

