#pragma once

struct Direct3D12SwapChain : GraphicsObject
{
    Direct3D12SwapChain(Direct3D12GraphicsDevice* graphicsDevice)
    {
        GraphicsDevice = graphicsDevice;
        GraphicsApi = GraphicsApi_Direct3D12;

        RenderBuffers[0] = nullptr;
        RenderBuffers[1] = nullptr;
        RenderBuffers[2] = nullptr;
    }

    Direct3D12GraphicsDevice* GraphicsDevice;
    ComPtr<IDXGISwapChain4> DeviceObject;
    Direct3D12CommandQueue* CommandQueue;
    HANDLE WaitHandle;
    uint32_t Width = 0;
    uint32_t Height = 0;

    SwapChainFormat Format;
    Direct3D12Texture* RenderBuffers[3];
    uint32_t RenderBufferCount;
};