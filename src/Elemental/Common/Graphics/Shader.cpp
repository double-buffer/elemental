#include "Elemental.h"
#include "GraphicsCommon.h"

ElemAPI ElemShaderLibrary ElemCreateShaderLibrary(ElemDataSpan shaderLibraryData)
{
    DispatchReturnGraphicsFunction(CreateShaderLibrary, shaderLibraryData);
}

ElemAPI void ElemFreeShaderLibrary(ElemShaderLibrary shaderLibrary)
{
    DispatchGraphicsFunction(FreeShaderLibrary, shaderLibrary);
}

ElemAPI ElemPipelineState ElemCompileGraphicsPipelineState(ElemGraphicsDevice graphicsDevice, const ElemGraphicsPipelineStateParameters* parameters)
{
    DispatchReturnGraphicsFunction(CompileGraphicsPipelineState, graphicsDevice, parameters);
}

ElemAPI void ElemFreePipelineState(ElemPipelineState pipelineState)
{
    DispatchGraphicsFunction(FreePipelineState, pipelineState);
}
