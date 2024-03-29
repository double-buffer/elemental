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

ElemAPI void ElemDispatchMesh(ElemCommandList commandList, uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ)
{
    DispatchGraphicsFunction(DispatchMesh, commandList, threadGroupCountX, threadGroupCountY, threadGroupCountZ);
}
