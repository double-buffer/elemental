#include "Elemental.h"
#include "GraphicsCommon.h"

ElemAPI ElemCommandQueue ElemCreateCommandQueue(ElemGraphicsDevice graphicsDevice, ElemCommandQueueType type, const ElemCommandQueueOptions* options)
{
    DispatchReturnGraphicsFunction(CreateCommandQueue, graphicsDevice, type, options);
}

ElemAPI void ElemFreeCommandQueue(ElemCommandQueue commandQueue)
{
    DispatchGraphicsFunction(FreeCommandQueue, commandQueue);
}

ElemAPI ElemCommandList ElemCreateCommandList(ElemCommandQueue commandQueue, const ElemCommandListOptions* options)
{
    DispatchReturnGraphicsFunction(CreateCommandList, commandQueue, options);
}

ElemAPI void ElemCommitCommandList(ElemCommandList commandList)
{
    DispatchGraphicsFunction(CommitCommandList, commandList);
}

ElemAPI ElemFence ElemExecuteCommandList(ElemCommandQueue commandQueue, ElemCommandList commandList, const ElemExecuteCommandListOptions* options)
{
    DispatchReturnGraphicsFunction(ExecuteCommandList, commandQueue, commandList, options);
}

ElemAPI ElemFence ElemExecuteCommandLists(ElemCommandQueue commandQueue, ElemCommandListSpan commandLists, const ElemExecuteCommandListOptions* options)
{
    DispatchReturnGraphicsFunction(ExecuteCommandLists, commandQueue, commandLists, options);
}
