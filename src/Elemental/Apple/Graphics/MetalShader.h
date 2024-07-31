#pragma once

#include "Elemental.h"
#include "SystemSpan.h"
#include "Graphics/ShaderReader.h"

struct MetalShaderLibraryData
{
    NS::SharedPtr<MTL::Library> MetalLibrary;
    Span<NS::SharedPtr<MTL::Library>> GraphicsShaders;
    ReadOnlySpan<Shader> Shaders;
};

struct MetalShaderMetaData
{
    uint32_t ThreadSizeX;
    uint32_t ThreadSizeY;
    uint32_t ThreadSizeZ;
};

struct MetalPipelineStateData
{
    NS::SharedPtr<MTL::RenderPipelineState> RenderPipelineState;
    NS::SharedPtr<MTL::ComputePipelineState> ComputePipelineState;
    MetalShaderMetaData AmplificaitonShaderMetaData;
    MetalShaderMetaData MeshShaderMetaData;
    MetalShaderMetaData ComputeShaderMetaData;
};

struct MetalPipelineStateDataFull
{
    uint32_t reserved;
};

MetalShaderLibraryData* GetMetalShaderLibraryData(ElemShaderLibrary shaderLibrary);

MetalPipelineStateData* GetMetalPipelineStateData(ElemPipelineState pipelineState);
MetalPipelineStateDataFull* GetMetalPipelineStateDataFull(ElemPipelineState pipelineState);

bool CheckMetalCommandEncoderType(const MetalCommandListData* commandListData, MetalCommandEncoderType type);

ElemShaderLibrary MetalCreateShaderLibrary(ElemGraphicsDevice graphicsDevice, ElemDataSpan shaderLibraryData);
void MetalFreeShaderLibrary(ElemShaderLibrary shaderLibrary);
ElemPipelineState MetalCompileGraphicsPipelineState(ElemGraphicsDevice graphicsDevice, const ElemGraphicsPipelineStateParameters* parameters);
ElemPipelineState MetalCompileComputePipelineState(ElemGraphicsDevice graphicsDevice, const ElemComputePipelineStateParameters* parameters);
void MetalFreePipelineState(ElemPipelineState pipelineState);
void MetalBindPipelineState(ElemCommandList commandList, ElemPipelineState pipelineState);
void MetalPushPipelineStateConstants(ElemCommandList commandList, uint32_t offsetInBytes, ElemDataSpan data); 

void MetalDispatchCompute(ElemCommandList commandList, uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ);
