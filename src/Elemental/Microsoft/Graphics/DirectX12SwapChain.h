#pragma once

#include "Elemental.h"

#define DIRECTX12_MAX_SWAPCHAIN_BUFFERS 3

struct DirectX12SwapChainData
{
    ComPtr<IDXGISwapChain4> DeviceObject;
    HANDLE WaitHandle;
    ElemTexture BackBufferTextures[DIRECTX12_MAX_SWAPCHAIN_BUFFERS];
};

struct DirectX12SwapChainDataFull
{
    ElemGraphicsDevice GraphicsDevice;
    ElemCommandQueue CommandQueue;
};

DirectX12SwapChainData* GetDirectX12SwapChainData(ElemSwapChain swapChain);
DirectX12SwapChainDataFull* GetDirectX12SwapChainDataFull(ElemSwapChain swapChain);

ElemSwapChain DirectX12CreateSwapChain(ElemCommandQueue commandQueue, ElemWindow window, const ElemSwapChainOptions* options);
void DirectX12FreeSwapChain(ElemSwapChain swapChain);
void DirectX12ResizeSwapChain(ElemSwapChain swapChain, uint32_t width, uint32_t height);
void DirectX12PresentSwapChain(ElemSwapChain swapChain);
void DirectX12WaitForSwapChainOnCpu(ElemSwapChain swapChain);
