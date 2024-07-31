#include "Elemental.h"
#include "GraphicsCommon.h"

ElemAPI ElemShaderLibrary ElemCreateShaderLibrary(ElemGraphicsDevice graphicsDevice, ElemDataSpan shaderLibraryData)
{
    DispatchReturnGraphicsFunction(CreateShaderLibrary, graphicsDevice, shaderLibraryData);
}

ElemAPI void ElemFreeShaderLibrary(ElemShaderLibrary shaderLibrary)
{
    DispatchGraphicsFunction(FreeShaderLibrary, shaderLibrary);
}

ElemAPI ElemPipelineState ElemCompileGraphicsPipelineState(ElemGraphicsDevice graphicsDevice, const ElemGraphicsPipelineStateParameters* parameters)
{
    DispatchReturnGraphicsFunction(CompileGraphicsPipelineState, graphicsDevice, parameters);
}

ElemAPI ElemPipelineState ElemCompileComputePipelineState(ElemGraphicsDevice graphicsDevice, const ElemComputePipelineStateParameters* parameters)
{
    DispatchReturnGraphicsFunction(CompileComputePipelineState, graphicsDevice, parameters);
}

ElemAPI void ElemFreePipelineState(ElemPipelineState pipelineState)
{
    DispatchGraphicsFunction(FreePipelineState, pipelineState);
}

ElemAPI void ElemBindPipelineState(ElemCommandList commandList, ElemPipelineState pipelineState)
{
    DispatchGraphicsFunction(BindPipelineState, commandList, pipelineState);
}

ElemAPI void ElemPushPipelineStateConstants(ElemCommandList commandList, uint32_t offsetInBytes, ElemDataSpan data)
{
    DispatchGraphicsFunction(PushPipelineStateConstants, commandList, offsetInBytes, data);
}

ElemAPI void ElemDispatchCompute(ElemCommandList commandList, uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ)
{
    DispatchGraphicsFunction(DispatchCompute, commandList, threadGroupCountX, threadGroupCountY, threadGroupCountZ);
}
