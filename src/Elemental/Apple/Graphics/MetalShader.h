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

ElemShaderLibrary MetalCreateShaderLibrary(ElemGraphicsDevice graphicsDevice, ElemDataSpan shaderLibraryData);
void MetalFreeShaderLibrary(ElemShaderLibrary shaderLibrary);
ElemPipelineState MetalCompileGraphicsPipelineState(ElemGraphicsDevice graphicsDevice, const ElemGraphicsPipelineStateParameters* parameters);
void MetalFreePipelineState(ElemPipelineState pipelineState);
void MetalBindPipelineState(ElemCommandList commandList, ElemPipelineState pipelineState);
void MetalPushPipelineStateConstants(ElemCommandList commandList, uint32_t offsetInBytes, ElemDataSpan data); 
