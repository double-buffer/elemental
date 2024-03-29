#pragma once

#include "Elemental.h"
#include "SystemSpan.h"

struct DirectX12ShaderLibraryData
{
    D3D12_SHADER_BYTECODE ShaderLibraryData;
    ReadOnlySpan<D3D12_SHADER_BYTECODE> GraphicsShaders; // HACK: This should be temporary, in the future DX12 should only libs for all stages
};

struct DirectX12PipelineStateData
{
    ComPtr<ID3D12StateObject> DeviceObject;
};

struct DirectX12PipelineStateDataFull
{
    uint32_t Reserved;
};

DirectX12ShaderLibraryData* GetDirectX12ShaderLibraryData(ElemShaderLibrary shaderLibrary);

DirectX12PipelineStateData* GetDirectX12PipelineStateData(ElemPipelineState pipelineState);
DirectX12PipelineStateDataFull* GetDirectX12PipelineStateDataFull(ElemPipelineState pipelineState);

ElemShaderLibrary DirectX12CreateShaderLibrary(ElemDataSpan shaderLibraryData);
void DirectX12FreeShaderLibrary(ElemShaderLibrary shaderLibrary);
ElemPipelineState DirectX12CompileGraphicsPipelineState(ElemGraphicsDevice graphicsDevice, const ElemGraphicsPipelineStateParameters* parameters);
void DirectX12FreePipelineState(ElemPipelineState pipelineState);
