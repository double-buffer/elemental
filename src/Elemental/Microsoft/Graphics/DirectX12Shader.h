#pragma once

#include "Elemental.h"
#include "SystemSpan.h"
#include "DirectX12CommandList.h"
#include "Graphics/ShaderReader.h"

struct DirectX12ShaderLibraryData
{
    D3D12_SHADER_BYTECODE ShaderLibraryData;
    ReadOnlySpan<Shader> GraphicsShaders; // HACK: This should be temporary, in the future DX12 should only libs for all stages
};

struct DirectX12PipelineStateData
{
    ComPtr<ID3D12PipelineState> PipelineState;
    D3D12_PROGRAM_IDENTIFIER ProgramIdentifier;
    DirectX12PipelineStateType PipelineStateType;
};

struct DirectX12PipelineStateDataFull
{
    ComPtr<ID3D12StateObject> StateObject;
};

DirectX12ShaderLibraryData* GetDirectX12ShaderLibraryData(ElemShaderLibrary shaderLibrary);

DirectX12PipelineStateData* GetDirectX12PipelineStateData(ElemPipelineState pipelineState);
DirectX12PipelineStateDataFull* GetDirectX12PipelineStateDataFull(ElemPipelineState pipelineState);

ElemShaderLibrary DirectX12CreateShaderLibrary(ElemGraphicsDevice graphicsDevice, ElemDataSpan shaderLibraryData);
void DirectX12FreeShaderLibrary(ElemShaderLibrary shaderLibrary);
ElemPipelineState DirectX12CompileGraphicsPipelineState(ElemGraphicsDevice graphicsDevice, const ElemGraphicsPipelineStateParameters* parameters);
ElemPipelineState DirectX12CompileComputePipelineState(ElemGraphicsDevice graphicsDevice, const ElemComputePipelineStateParameters* parameters);
void DirectX12FreePipelineState(ElemPipelineState pipelineState);
void DirectX12BindPipelineState(ElemCommandList commandList, ElemPipelineState pipelineState);
void DirectX12PushPipelineStateConstants(ElemCommandList commandList, uint32_t offsetInBytes, ElemDataSpan data); 

void DirectX12DispatchCompute(ElemCommandList commandList, uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ);


// TODO: TEMP
#define PsoSubObject(name, subObjectType, subObject) 	struct alignas(void*) Def##name \
						{ \
							D3D12_PIPELINE_STATE_SUBOBJECT_TYPE type; \
							subObject innerObject; \
 \
							Def##name() noexcept : type(subObjectType), innerObject()  \
							{ \
							} \
 \
							Def##name(subObject const& i) : type(subObjectType), innerObject(i) {} \
							Def##name& operator=(subObject const& i) { innerObject = i; return *this; } \
							operator subObject() const { return innerObject; } \
							operator subObject() { return innerObject; } \
						} name; 

struct GraphicsPso
{
public:
	PsoSubObject(RootSignature, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE, ID3D12RootSignature*);
	PsoSubObject(MS, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_MS, D3D12_SHADER_BYTECODE);
	PsoSubObject(PS, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS, D3D12_SHADER_BYTECODE);
	PsoSubObject(RenderTargets, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RENDER_TARGET_FORMATS, D3D12_RT_FORMAT_ARRAY);
	PsoSubObject(SampleDesc, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_DESC, DXGI_SAMPLE_DESC);
	PsoSubObject(RasterizerState, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER2, D3D12_RASTERIZER_DESC2);
	PsoSubObject(DepthStencilFormat, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL_FORMAT, DXGI_FORMAT);
	PsoSubObject(DepthStencilState, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL2, D3D12_DEPTH_STENCIL_DESC2);
	PsoSubObject(BlendState, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_BLEND, D3D12_BLEND_DESC);
};
