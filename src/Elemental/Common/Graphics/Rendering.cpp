#include "Elemental.h"
#include "GraphicsCommon.h"

ElemAPI void ElemBeginRenderPass(ElemCommandList commandList, const ElemBeginRenderPassParameters* parameters)
{
    DispatchGraphicsFunction(BeginRenderPass, commandList, parameters);
}

ElemAPI void ElemEndRenderPass(ElemCommandList commandList)
{
    DispatchGraphicsFunction(EndRenderPass, commandList);
}
