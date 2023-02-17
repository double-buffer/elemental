#pragma once
#include "WindowsCommon.h"
#include "Direct3D12BaseGraphicsObject.h"

struct Direct3D12SwapChain : Direct3D12BaseGraphicsObject
{
    Direct3D12SwapChain(BaseGraphicsService* graphicsService, Direct3D12GraphicsDevice* graphicsDevice) : Direct3D12BaseGraphicsObject(graphicsService, graphicsDevice)
    {
    }

    ComPtr<IDXGISwapChain4> DeviceObject;
    Direct3D12CommandQueue* CommandQueue;
    HANDLE WaitHandle;
};