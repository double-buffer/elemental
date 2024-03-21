#include "Elemental.h"
#include "GraphicsCommon.h"

ElemAPI ElemSwapChain ElemCreateSwapChain(ElemCommandQueue commandQueue, ElemWindow window, const ElemSwapChainOptions* options)
{
    DispatchReturnGraphicsFunction(CreateSwapChain, commandQueue, window, options);
}

ElemAPI void ElemFreeSwapChain(ElemSwapChain swapChain)
{
    DispatchGraphicsFunction(FreeSwapChain, swapChain);
}

ElemAPI void ElemResizeSwapChain(ElemSwapChain swapChain, uint32_t width, uint32_t height)
{
    DispatchGraphicsFunction(ResizeSwapChain, swapChain, width, height);
}

ElemAPI void ElemPresentSwapChain(ElemSwapChain swapChain)
{
    DispatchGraphicsFunction(PresentSwapChain, swapChain);
}

ElemAPI void ElemWaitForSwapChainOnCpu(ElemSwapChain swapChain)
{
    DispatchGraphicsFunction(WaitForSwapChainOnCpu, swapChain);
}
