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
    ElemCommandListSpan commandListSpan = 
    {
        .Items = &commandList,
        .Length = 1
    };

    DispatchReturnGraphicsFunction(ExecuteCommandLists, commandQueue, commandListSpan, options);
}

ElemAPI ElemFence ElemExecuteCommandLists(ElemCommandQueue commandQueue, ElemCommandListSpan commandLists, const ElemExecuteCommandListOptions* options)
{
    DispatchReturnGraphicsFunction(ExecuteCommandLists, commandQueue, commandLists, options);
}

ElemAPI void ElemWaitForFenceOnCpu(ElemFence fence)
{
    DispatchGraphicsFunction(WaitForFenceOnCpu, fence);
}
