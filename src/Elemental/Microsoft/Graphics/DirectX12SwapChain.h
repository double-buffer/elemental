#pragma once

#include "Elemental.h"

struct DirectX12SwapChainData
{
    ComPtr<IDXGISwapChain4> DeviceObject;
    HANDLE WaitHandle;
};

struct DirectX12SwapChainDataFull
{
    ElemCommandQueue CommandQueue;
};

DirectX12SwapChainData* GetDirectX12SwapChainData(ElemSwapChain swapChain);
DirectX12SwapChainDataFull* GetDirectX12SwapChainDataFull(ElemSwapChain swapChain);

ElemSwapChain DirectX12CreateSwapChain(ElemCommandQueue commandQueue, ElemWindow window, const ElemSwapChainOptions* options);
void DirectX12FreeSwapChain(ElemSwapChain swapChain);
void DirectX12ResizeSwapChain(ElemSwapChain swapChain, uint32_t width, uint32_t height);
void DirectX12PresentSwapChain(ElemSwapChain swapChain);
void DirectX12WaitForSwapChainOnCpu(ElemSwapChain swapChain);
