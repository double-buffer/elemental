#pragma once

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
