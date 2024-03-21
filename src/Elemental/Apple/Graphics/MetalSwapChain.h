#pragma once

#include "Elemental.h"

struct MetalSwapChainData
{
};

struct MetalSwapChainDataFull
{
};

MetalSwapChainData* GetMetalSwapChainData(ElemSwapChain commandQueue);
MetalSwapChainDataFull* GetMetalSwapChainDataFull(ElemSwapChain commandQueue);

ElemSwapChain MetalCreateSwapChain(ElemCommandQueue commandQueue, ElemWindow window, const ElemSwapChainOptions* options);
void MetalFreeSwapChain(ElemSwapChain swapChain);
void MetalResizeSwapChain(ElemSwapChain swapChain, uint32_t width, uint32_t height);
void MetalPresentSwapChain(ElemSwapChain swapChain);
void MetalWaitForSwapChainOnCpu(ElemSwapChain swapChain);
