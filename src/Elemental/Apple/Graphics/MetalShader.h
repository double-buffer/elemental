#pragma once

#include "Elemental.h"

struct MetalShaderLibraryData
{
};

struct MetalShaderLibraryDataFull
{
};

struct MetalPipelineStateData
{
};

struct MetalPipelineStateDataFull
{
};

MetalShaderLibraryData* GetMetalShaderLibraryData(ElemShaderLibrary shaderLibrary);
MetalShaderLibraryDataFull* GetMetalShaderLibraryDataFull(ElemTexture shaderLibrary);

MetalPipelineStateData* GetMetalPipelineStateData(ElemPipelineState pipelineState);
MetalPipelineStateDataFull* GetMetalPipelineStateDataFull(ElemPipelineState pipelineState);

ElemShaderLibrary MetalCreateShaderLibrary(ElemDataSpan shaderLibraryData);
void MetalFreeShaderLibrary(ElemShaderLibrary shaderLibrary);
ElemPipelineState MetalCompileGraphicsPipelineState(ElemGraphicsDevice graphicsDevice, const ElemGraphicsPipelineStateParameters* parameters);
void MetalFreePipelineState(ElemPipelineState pipelineState);
