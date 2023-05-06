#pragma once

struct Direct3D12CommandQueue : Direct3D12BaseGraphicsObject
{
    Direct3D12CommandQueue(BaseGraphicsService* graphicsService, Direct3D12GraphicsDevice* graphicsDevice) : Direct3D12BaseGraphicsObject(graphicsService, graphicsDevice)
    {
    }

    D3D12_COMMAND_LIST_TYPE CommandListType;
    ComPtr<ID3D12CommandQueue> DeviceObject;
    ComPtr<ID3D12Fence1> Fence;
    uint64_t FenceValue = 0;
    uint64_t LastCompletedFenceValue = 0;
};