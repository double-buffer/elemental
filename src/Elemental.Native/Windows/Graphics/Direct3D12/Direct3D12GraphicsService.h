#pragma once
#include "PreCompiledHeader.h"
#include "BaseGraphicsService.h"
#include "BaseGraphicsObject.h"
#include "SystemFunctions.h"

struct Direct3D12GraphicsDevice;

#define D3D12SDKVersion 610

#define MAX_DIRECT3D12_GRAPHICS_DEVICES 64
#define MAX_DIRECT3D12_COMMAND_ALLOCATORS 64
#define MAX_DIRECT3D12_COMMAND_LISTS 64

#include "Win32Window.h"
#include "Direct3D12CommandQueue.h"
#include "Direct3D12CommandList.h"
#include "Direct3D12Shader.h"
#include "Direct3D12Texture.h"
#include "Direct3D12SwapChain.h"
#include "Direct3D12GraphicsDevice.h"

// TODO: Prefix structures with Direct3D12 like in vulkan!

class Direct3D12GraphicsService : BaseGraphicsService
{
public:
    Direct3D12GraphicsService(GraphicsServiceOptions* options);
    ~Direct3D12GraphicsService();

    void GetAvailableGraphicsDevices(GraphicsDeviceInfo* graphicsDevices, int* count) override;
    void* CreateGraphicsDevice(GraphicsDeviceOptions* options) override;
    void FreeGraphicsDevice(void* graphicsDevicePointer) override;
    GraphicsDeviceInfo GetGraphicsDeviceInfo(void* graphicsDevicePointer) override;
    
    void* CreateCommandQueue(void* graphicsDevicePointer, CommandQueueType type) override;
    void FreeCommandQueue(void* commandQueuePointer) override;
    void SetCommandQueueLabel(void* commandQueuePointer, uint8_t* label) override;
    
    void* CreateCommandList(void* commandQueuePointer) override;
    void FreeCommandList(void* commandListPointer) override;
    void SetCommandListLabel(void* commandListPointer, uint8_t* label) override;
    void CommitCommandList(void* commandList) override;
    
    Fence ExecuteCommandLists(void* commandQueuePointer, void** commandLists, int32_t commandListCount, Fence* fencesToWait, int32_t fenceToWaitCount) override;
    void WaitForFenceOnCpu(Fence fence) override;
    void ResetCommandAllocation(void* graphicsDevicePointer) override;
    
    void FreeTexture(void* texturePointer) override;
    
    void* CreateSwapChain(void* windowPointer, void* commandQueuePointer, SwapChainOptions* options) override;
    void FreeSwapChain(void* swapChainPointer) override;
    void ResizeSwapChain(void* swapChainPointer, int width, int height) override;
    void* GetSwapChainBackBufferTexture(void* swapChainPointer) override;
    void PresentSwapChain(void* swapChainPointer) override;
    void WaitForSwapChainOnCpu(void* swapChainPointer) override;

    void* CreateShader(void* graphicsDevicePointer, ShaderPart* shaderParts, int32_t shaderPartCount) override;
    void FreeShader(void* shaderPointer) override;
    
    void BeginRenderPass(void* commandListPointer, RenderPassDescriptor* renderPassDescriptor) override;
    void EndRenderPass(void* commandListPointer) override;
    
    void SetShader(void* commandListPointer, void* shaderPointer) override;
    void SetShaderConstants(void* commandListPointer, uint32_t slot, void* constantValues, int32_t constantValueCount) override;

    void DispatchMesh(void* commandListPointer, uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ) override;

private:
    GraphicsDiagnostics _graphicsDiagnostics;
    ComPtr<ID3D12SDKConfiguration> _sdkConfiguration;
    ComPtr<IDXGIFactory6> _dxgiFactory; 
    ComPtr<ID3D12Debug6> _debugInterface;
    ComPtr<ID3D12InfoQueue1> _debugInfoQueue;
    ComPtr<IDXGIDebug1> _dxgiDebugInterface;
    HANDLE _globalFenceEvent;
    uint32_t _currentDeviceInternalId = 0;

    GraphicsDeviceInfo ConstructGraphicsDeviceInfo(DXGI_ADAPTER_DESC3 adapterDescription);
    uint64_t GetDeviceId(DXGI_ADAPTER_DESC3 adapterDescription);

    CommandAllocatorPoolItem* GetCommandAllocator(Direct3D12CommandQueue* commandQueue);
    void UpdateCommandAllocatorFence(Direct3D12CommandList* commandList, uint64_t fenceValue);
    Direct3D12CommandList* GetCommandList(Direct3D12CommandQueue* commandQueue, CommandAllocatorPoolItem* commandAllocatorPoolItem);
    void PushFreeCommandList(Direct3D12CommandQueue* commandQueue, Direct3D12CommandList* commandList);
    Fence CreateCommandQueueFence(Direct3D12CommandQueue* commandQueue);

    void CreateSwapChainBackBuffers(Direct3D12SwapChain* swapChain);

    void InitRenderPassRenderTarget(Direct3D12CommandList* commandList, D3D12_RENDER_PASS_RENDER_TARGET_DESC* renderPassRenderTargetDesc, RenderPassRenderTarget* renderTarget);
    
    // TODO: Move that function to common code (abstract shader for all graphics API)
    uint64_t ComputeRenderPipelineStateHash(Direct3D12Shader* shader, RenderPassDescriptor* renderPassDescriptor);
    ComPtr<ID3D12PipelineState> CreateRenderPipelineState(Direct3D12Shader* shader, RenderPassDescriptor* renderPassDescriptor);
};

NativeWindowSize Native_GetWindowRenderSize(Win32Window* nativeWindow);
static void DebugReportCallback(D3D12_MESSAGE_CATEGORY Category, D3D12_MESSAGE_SEVERITY Severity, D3D12_MESSAGE_ID ID, LPCSTR pDescription, void* pContext);
static void Direct3D12DeletePipelineCacheItem(uint64_t key, void* data);

thread_local DeviceCommandAllocators CommandAllocators[MAX_DIRECT3D12_GRAPHICS_DEVICES];


#define PsoSubObject(name, subObjectType, subObject) 	struct alignas(void*) Def##name \
						{ \
						private: \
							D3D12_PIPELINE_STATE_SUBOBJECT_TYPE type; \
							subObject innerObject; \
 \
						public: \
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