#include "Elemental.h"
#include "GraphicsCommon.h"

ElemAPI void ElemBeginRenderPass(ElemCommandList commandList, const ElemBeginRenderPassOptions* options)
{
    DispatchGraphicsFunction(BeginRenderPass, commandList, options);
}

ElemAPI void ElemEndRenderPass(ElemCommandList commandList)
{
    DispatchGraphicsFunction(EndRenderPass, commandList);
}
