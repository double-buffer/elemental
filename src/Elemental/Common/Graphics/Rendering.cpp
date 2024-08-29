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

ElemAPI void ElemSetViewport(ElemCommandList commandList, const ElemViewport* viewport)
{
    ElemViewportSpan viewportSpan = 
    {
        .Items = (ElemViewport*)viewport,
        .Length = 1
    };

    DispatchGraphicsFunction(SetViewports, commandList, viewportSpan);
}

ElemAPI void ElemSetViewports(ElemCommandList commandList, ElemViewportSpan viewports)
{
    DispatchGraphicsFunction(SetViewports, commandList, viewports);
}

ElemAPI void ElemSetScissorRectangle(ElemCommandList commandList, const ElemRectangle* rectangle)
{
    ElemRectangleSpan rectangleSpan = 
    {
        .Items = (ElemRectangle*)rectangle,
        .Length = 1
    };

    DispatchGraphicsFunction(SetScissorRectangles, commandList, rectangleSpan);
}

ElemAPI void ElemSetScissorRectangles(ElemCommandList commandList, ElemRectangleSpan rectangles)
{
    DispatchGraphicsFunction(SetScissorRectangles, commandList, rectangles);
}

ElemAPI void ElemDispatchMesh(ElemCommandList commandList, uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ)
{
    DispatchGraphicsFunction(DispatchMesh, commandList, threadGroupCountX, threadGroupCountY, threadGroupCountZ);
}
