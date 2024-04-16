#include "Elemental.h"
#include "GraphicsCommon.h"

ElemAPI ElemSwapChain ElemCreateSwapChain(ElemCommandQueue commandQueue, ElemWindow window, ElemSwapChainUpdateHandlerPtr updateHandler, const ElemSwapChainOptions* options)
{
    DispatchReturnGraphicsFunction(CreateSwapChain, commandQueue, window, updateHandler, options);
}

ElemAPI void ElemFreeSwapChain(ElemSwapChain swapChain)
{
    DispatchGraphicsFunction(FreeSwapChain, swapChain);
}

ElemAPI ElemSwapChainInfo ElemGetSwapChainInfo(ElemSwapChain swapChain)
{
    DispatchReturnGraphicsFunction(GetSwapChainInfo, swapChain);
}

ElemAPI void ElemPresentSwapChain(ElemSwapChain swapChain)
{
    DispatchGraphicsFunction(PresentSwapChain, swapChain);
}
