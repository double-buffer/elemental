#include "MetalSwapChain.h"
#include "MetalGraphicsDevice.h"
#include "SystemDataPool.h"

#define METAL_MAX_SWAPCHAINS 10u

SystemDataPool<MetalSwapChainData, MetalSwapChainDataFull> metalSwapChainPool;

void InitMetalSwapChainMemory()
{
    if (!metalSwapChainPool.Storage)
    {
        metalSwapChainPool = SystemCreateDataPool<MetalSwapChainData, MetalSwapChainDataFull>(MetalGraphicsMemoryArena, METAL_MAX_SWAPCHAINS);
    }
}

MetalSwapChainData* GetMetalSwapChainData(ElemSwapChain swapChain)
{
    return SystemGetDataPoolItem(metalSwapChainPool, swapChain);
}

MetalSwapChainDataFull* GetMetalSwapChainDataFull(ElemSwapChain swapChain)
{
    return SystemGetDataPoolItemFull(metalSwapChainPool, swapChain);
}

ElemSwapChain MetalCreateSwapChain(ElemCommandQueue commandQueue, ElemWindow window, const ElemSwapChainOptions* options)
{
    InitMetalSwapChainMemory();

    return ELEM_HANDLE_NULL;
}

void MetalFreeSwapChain(ElemSwapChain swapChain)
{
}

void MetalResizeSwapChain(ElemSwapChain swapChain, uint32_t width, uint32_t height)
{
}

ElemTexture MetalGetSwapChainBackBufferTexture(ElemSwapChain swapChain)
{
    return ELEM_HANDLE_NULL;
}

void MetalPresentSwapChain(ElemSwapChain swapChain)
{
}

void MetalWaitForSwapChainOnCpu(ElemSwapChain swapChain)
{
}
