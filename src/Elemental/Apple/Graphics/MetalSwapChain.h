#pragma once

#include "Elemental.h"

struct MetalSwapChainData
{
    NS::SharedPtr<CA::MetalLayer> DeviceObject;
    NS::SharedPtr<CA::MetalDrawable> BackBufferDrawable;
    ElemTexture BackBufferTexture;
    dispatch_semaphore_t WaitSemaphore;
    ElemCommandQueue CommandQueue;
    ElemGraphicsDevice GraphicsDevice;
};

struct MetalSwapChainDataFull
{
    uint32_t reserved;
};

MetalSwapChainData* GetMetalSwapChainData(ElemSwapChain swapChain);
MetalSwapChainDataFull* GetMetalSwapChainDataFull(ElemSwapChain swapChain);

ElemSwapChain MetalCreateSwapChain(ElemCommandQueue commandQueue, ElemWindow window, const ElemSwapChainOptions* options);
void MetalFreeSwapChain(ElemSwapChain swapChain);
ElemSwapChainInfo MetalGetSwapChainInfo(ElemSwapChain swapChain);
void MetalResizeSwapChain(ElemSwapChain swapChain, uint32_t width, uint32_t height);
void MetalPresentSwapChain(ElemSwapChain swapChain);
void MetalWaitForSwapChainOnCpu(ElemSwapChain swapChain);
