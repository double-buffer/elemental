#include "Elemental.h"
#include "GraphicsCommon.h"

ElemAPI ElemGraphicsCommandQueue ElemCreateGraphicsCommandQueue(ElemGraphicsDevice graphicsDevice, ElemGraphicsCommandQueueType type, const ElemGraphicsCommandQueueOptions* options)
{
    DispatchReturnGraphicsFunction(CreateGraphicsCommandQueue, graphicsDevice, type, options);
}

ElemAPI void ElemFreeGraphicsCommandQueue(ElemGraphicsCommandQueue commandQueue)
{
    DispatchGraphicsFunction(FreeGraphicsCommandQueue, commandQueue);
}
