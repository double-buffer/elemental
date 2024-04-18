#pragma once

#include "Elemental.h"

#define DIRECTX12_MAX_SWAPCHAIN_BUFFERS 3

// TODO: Review structure
struct DirectX12SwapChainData
{
    ComPtr<IDXGISwapChain4> DeviceObject;
    ElemCommandQueue CommandQueue;
    ElemWindow Window;
    HANDLE WaitHandle;
    ElemTexture BackBufferTextures[DIRECTX12_MAX_SWAPCHAIN_BUFFERS];
    ElemSwapChainUpdateHandlerPtr UpdateHandler;
    void* UpdatePayload;
    LARGE_INTEGER CreationTimestamp;
    LARGE_INTEGER PreviousTargetPresentationTimestamp;
    uint32_t Width;
    uint32_t Height;
    ElemTextureFormat Format;
    bool PresentCalled;
};

struct DirectX12SwapChainDataFull
{
    ElemGraphicsDevice GraphicsDevice;
};

DirectX12SwapChainData* GetDirectX12SwapChainData(ElemSwapChain swapChain);
DirectX12SwapChainDataFull* GetDirectX12SwapChainDataFull(ElemSwapChain swapChain);

ElemSwapChain DirectX12CreateSwapChain(ElemCommandQueue commandQueue, ElemWindow window, ElemSwapChainUpdateHandlerPtr updateHandler, const ElemSwapChainOptions* options);
void DirectX12FreeSwapChain(ElemSwapChain swapChain);
ElemSwapChainInfo DirectX12GetSwapChainInfo(ElemSwapChain swapChain);
void DirectX12ResizeSwapChain(ElemSwapChain swapChain, uint32_t width, uint32_t height);
void DirectX12PresentSwapChain(ElemSwapChain swapChain);
void DirectX12WaitForSwapChainOnCpu(ElemSwapChain swapChain);
