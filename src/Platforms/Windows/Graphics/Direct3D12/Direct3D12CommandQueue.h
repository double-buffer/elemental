#pragma once
#include "WindowsCommon.h"
#include "Direct3D12BaseGraphicsObject.h"

#include <stack>

struct Direct3D12CommandQueue : Direct3D12BaseGraphicsObject
{
    Direct3D12CommandQueue(BaseGraphicsService* graphicsService, Direct3D12GraphicsDevice* graphicsDevice) : Direct3D12BaseGraphicsObject(graphicsService, graphicsDevice)
    {
    }

    ComPtr<ID3D12CommandQueue> DeviceObject;
    D3D12_COMMAND_LIST_TYPE CommandListType;
    std::stack<ComPtr<ID3D12GraphicsCommandList7>> AvailableCommandLists;
};