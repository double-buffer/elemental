#pragma once
#include "WindowsCommon.h"
#include "Direct3D12BaseGraphicsObject.h"

struct Direct3D12CommandList : Direct3D12BaseGraphicsObject
{
    Direct3D12CommandList(Direct3D12CommandQueue* commandQueue, BaseGraphicsService* graphicsService, Direct3D12GraphicsDevice* graphicsDevice) : Direct3D12BaseGraphicsObject(graphicsService, graphicsDevice)
    {
        CommandQueue = commandQueue;
    }

    ComPtr<ID3D12GraphicsCommandList7> DeviceObject;
    Direct3D12CommandQueue* CommandQueue;
};