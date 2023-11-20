#pragma once

#include "SystemMemory.h"
#include "Win32Window.h"
#include "GraphicsObject.h"
#include "Direct3D12Common.h"
#include "Direct3D12PipelineState.h"
#include "Direct3D12CommandQueue.h"
#include "Direct3D12CommandList.h"
#include "Direct3D12Shader.h"
#include "Direct3D12Texture.h"
#include "Direct3D12SwapChain.h"
#include "Direct3D12Shader.h"
#include "Direct3D12GraphicsDevice.h"

// TODO: We need to forward declare all the other functions
// TODO: Split that in multiple files

NativeWindowSize Native_GetWindowRenderSize(Win32Window* nativeWindow);
static void Direct3D12DebugReportCallback(D3D12_MESSAGE_CATEGORY Category, D3D12_MESSAGE_SEVERITY Severity, D3D12_MESSAGE_ID ID, LPCSTR pDescription, void* pContext);
static void Direct3D12DeletePipelineCacheItem(uint64_t key, void* data);

void Direct3D12WaitForFenceOnCpu(Fence fence);
Fence Direct3D12CreateCommandQueueFence(Direct3D12CommandQueue* commandQueue);

GraphicsDeviceInfo Direct3D12ConstructGraphicsDeviceInfo(DXGI_ADAPTER_DESC3 adapterDescription);
uint64_t Direct3D12GetDeviceId(DXGI_ADAPTER_DESC3 adapterDescription);

CommandAllocatorPoolItem* Direct3D12GetCommandAllocator(Direct3D12CommandQueue* commandQueue);
void Direct3D12UpdateCommandAllocatorFence(Direct3D12CommandList* commandList, uint64_t fenceValue);
Direct3D12CommandList* Direct3D12GetCommandList(Direct3D12CommandQueue* commandQueue, CommandAllocatorPoolItem* commandAllocatorPoolItem);
void Direct3D12PushFreeCommandList(Direct3D12CommandQueue* commandQueue, Direct3D12CommandList* commandList);

void Direct3D12CreateSwapChainBackBuffers(Direct3D12SwapChain* swapChain);

void Direct3D12InitRenderPassRenderTarget(Direct3D12CommandList* commandList, D3D12_RENDER_PASS_RENDER_TARGET_DESC* renderPassRenderTargetDesc, RenderPassRenderTarget* renderTarget);

// TODO: Move that function to common code (abstract shader for all graphics API)
uint64_t Direct3D12ComputeRenderPipelineStateHash(Direct3D12Shader* shader, RenderPassDescriptor* renderPassDescriptor);
ComPtr<ID3D12PipelineState> Direct3D12CreateRenderPipelineState(Direct3D12Shader* shader, RenderPassDescriptor* renderPassDescriptor);
