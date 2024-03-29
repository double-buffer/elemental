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
    ComPtr<ID3D12PipelineState> PipelineState;
    D3D12_PROGRAM_IDENTIFIER ProgramIdentifier;
};

struct DirectX12PipelineStateDataFull
{
    ComPtr<ID3D12StateObject> StateObject;
};

DirectX12ShaderLibraryData* GetDirectX12ShaderLibraryData(ElemShaderLibrary shaderLibrary);

DirectX12PipelineStateData* GetDirectX12PipelineStateData(ElemPipelineState pipelineState);
DirectX12PipelineStateDataFull* GetDirectX12PipelineStateDataFull(ElemPipelineState pipelineState);

ElemShaderLibrary DirectX12CreateShaderLibrary(ElemDataSpan shaderLibraryData);
void DirectX12FreeShaderLibrary(ElemShaderLibrary shaderLibrary);
ElemPipelineState DirectX12CompileGraphicsPipelineState(ElemGraphicsDevice graphicsDevice, const ElemGraphicsPipelineStateParameters* parameters);
void DirectX12FreePipelineState(ElemPipelineState pipelineState);
void DirectX12BindPipelineState(ElemCommandList commandList, ElemPipelineState pipelineState);
void DirectX12PushPipelineStateConstants(ElemCommandList commandList, uint32_t offsetInBytes, ElemDataSpan data); 



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
	PsoSubObject(AS, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_AS, D3D12_SHADER_BYTECODE);
	PsoSubObject(MS, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_MS, D3D12_SHADER_BYTECODE);
	PsoSubObject(PS, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS, D3D12_SHADER_BYTECODE);
	PsoSubObject(RenderTargets, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RENDER_TARGET_FORMATS, D3D12_RT_FORMAT_ARRAY);
	PsoSubObject(SampleDesc, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_DESC, DXGI_SAMPLE_DESC);
	PsoSubObject(RasterizerState, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER, D3D12_RASTERIZER_DESC);
	PsoSubObject(DepthStencilFormat, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL_FORMAT, DXGI_FORMAT);
	PsoSubObject(DepthStencilState, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL, D3D12_DEPTH_STENCIL_DESC);
	PsoSubObject(BlendState, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_BLEND, D3D12_BLEND_DESC);
};
