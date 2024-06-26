#pragma once

#include "Elemental.h"
#include "SystemSpan.h"

struct MetalShaderLibraryData
{
    NS::SharedPtr<MTL::Library> MetalLibrary;
    Span<NS::SharedPtr<MTL::Library>> GraphicsShaders;
};

struct MetalPipelineStateData
{
    NS::SharedPtr<MTL::RenderPipelineState> RenderPipelineState;
};

struct MetalPipelineStateDataFull
{
    uint32_t reserved;
};

MetalShaderLibraryData* GetMetalShaderLibraryData(ElemShaderLibrary shaderLibrary);

MetalPipelineStateData* GetMetalPipelineStateData(ElemPipelineState pipelineState);
MetalPipelineStateDataFull* GetMetalPipelineStateDataFull(ElemPipelineState pipelineState);

ElemShaderLibrary MetalCreateShaderLibrary(ElemGraphicsDevice graphicsDevice, ElemDataSpan shaderLibraryData);
void MetalFreeShaderLibrary(ElemShaderLibrary shaderLibrary);
ElemPipelineState MetalCompileGraphicsPipelineState(ElemGraphicsDevice graphicsDevice, const ElemGraphicsPipelineStateParameters* parameters);
void MetalFreePipelineState(ElemPipelineState pipelineState);
void MetalBindPipelineState(ElemCommandList commandList, ElemPipelineState pipelineState);
void MetalPushPipelineStateConstants(ElemCommandList commandList, uint32_t offsetInBytes, ElemDataSpan data); 
