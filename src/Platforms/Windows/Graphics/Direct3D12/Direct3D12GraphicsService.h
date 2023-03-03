#pragma once
#include "WindowsCommon.h"
#include "../../../Common/BaseGraphicsService.h"
#include "../../../Common/BaseGraphicsObject.h"
#include "../../../Common/SystemFunctions.h"

struct Direct3D12GraphicsDevice;

#include "../Win32Window.h"
#include "Direct3D12CommandQueue.h"
#include "Direct3D12CommandList.h"
#include "Direct3D12Texture.h"
#include "Direct3D12SwapChain.h"
#include "Direct3D12GraphicsDevice.h"

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
    
    void BeginRenderPass(void* commandListPointer, RenderPassDescriptor* renderPassDescriptor) override;
    void EndRenderPass(void* commandListPointer) override;

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

    ComPtr<ID3D12CommandAllocator> GetCommandAllocator(Direct3D12CommandQueue* commandQueue);
    void UpdateCommandAllocatorFence(Direct3D12CommandQueue* commandQueue);
    Direct3D12CommandList* GetCommandList(Direct3D12CommandQueue* commandQueue);
    void PushFreeCommandList(Direct3D12CommandQueue* commandQueue, Direct3D12CommandList* commandList);
    Fence CreateCommandQueueFence(Direct3D12CommandQueue* commandQueue);

    void CreateSwapChainBackBuffers(Direct3D12SwapChain* swapChain);
};

NativeWindowSize Native_GetWindowRenderSize(Win32Window* nativeWindow);
static void DebugReportCallback(D3D12_MESSAGE_CATEGORY Category, D3D12_MESSAGE_SEVERITY Severity, D3D12_MESSAGE_ID ID, LPCSTR pDescription, void* pContext);


// HACK: Remove map
#include <map>
thread_local std::map<uint32_t, DeviceCommandAllocators> CommandAllocators;