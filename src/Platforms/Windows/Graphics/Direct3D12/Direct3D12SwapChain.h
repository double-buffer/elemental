#pragma once
#include "WindowsCommon.h"
#include "Direct3D12BaseGraphicsObject.h"
#include "Direct3D12Texture.h"

struct Direct3D12SwapChain : Direct3D12BaseGraphicsObject
{
    Direct3D12SwapChain(BaseGraphicsService* graphicsService, Direct3D12GraphicsDevice* graphicsDevice) : Direct3D12BaseGraphicsObject(graphicsService, graphicsDevice)
    {
        RenderBuffers[0] = nullptr;
        RenderBuffers[1] = nullptr;
        RenderBuffers[2] = nullptr;
    }

    ComPtr<IDXGISwapChain4> DeviceObject;
    Direct3D12CommandQueue* CommandQueue;
    HANDLE WaitHandle;

    // TODO: Review and refactor
    Direct3D12Texture* RenderBuffers[3];
    uint32_t RenderBufferCount;
};