#define D3D12SDKVersion 610

#define MAX_DIRECT3D12_GRAPHICS_DEVICES 64
#define MAX_DIRECT3D12_COMMAND_ALLOCATORS 64
#define MAX_DIRECT3D12_COMMAND_LISTS 64

struct Direct3D12GraphicsDevice;

#include "Win32Window.h"
#include "GraphicsObject.h"
#include "Direct3D12PipelineState.h"
#include "Direct3D12CommandQueue.h"
#include "Direct3D12CommandList.h"
#include "Direct3D12Shader.h"
#include "Direct3D12Texture.h"
#include "Direct3D12SwapChain.h"
#include "Direct3D12Shader.h"
#include "Direct3D12GraphicsDevice.h"

#include "Direct3D12GraphicsService.h"
    
GraphicsDiagnostics _graphicsDiagnostics;
ComPtr<ID3D12SDKConfiguration> _sdkConfiguration;
ComPtr<IDXGIFactory6> _dxgiFactory; 
ComPtr<ID3D12Debug6> _debugInterface;
ComPtr<ID3D12InfoQueue1> _debugInfoQueue;
ComPtr<IDXGIDebug1> _dxgiDebugInterface;
uint32_t _currentDeviceInternalId = 0;
HANDLE _globalFenceEvent;
    
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

void Direct3D12InitGraphicsService(GraphicsServiceOptions* options)
{
    // HACK: For the moment we don't use ID3D12SDKConfiguration1 but we should so we can
    // select the SDK version without setting windows developer mode.
    // See: https://github.com/microsoft/DirectX-Specs/blob/master/d3d/IndependentDevices.md

    AssertIfFailed(D3D12GetInterface(CLSID_D3D12SDKConfiguration, IID_PPV_ARGS(_sdkConfiguration.GetAddressOf())));

    /*ComPtr<ID3D12SDKConfiguration1> sdkConfiguration1;
    AssertIfFailed(sdkConfiguration.As(&sdkConfiguration1));

    ComPtr<ID3D12DeviceFactory> deviceFactory;
    AssertIfFailed(sdkConfiguration1->CreateDeviceFactory(D3D12SDKVersion, D3D12SDKPath, IID_PPV_ARGS(deviceFactory.GetAddressOf())));*/

    AssertIfFailed(_sdkConfiguration->SetSDKVersion(D3D12SDKVersion, ".\\"));

    UINT createFactoryFlags = 0;
    bool sdkLayerExists = _waccess((SystemGetExecutableFolderPath() + L".\\D3D12SDKLayers.dll").c_str(), 0) == 0;
 
    if (options->GraphicsDiagnostics == GraphicsDiagnostics_Debug && sdkLayerExists)
    {
        printf("DirectX12 Debug Mode\n");

        AssertIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(_debugInterface.GetAddressOf())));

        if (_debugInterface)
        {
            _debugInterface->EnableDebugLayer();
            _debugInterface->SetEnableSynchronizedCommandQueueValidation(true);
        }

        AssertIfFailed(DXGIGetDebugInterface1(0, IID_PPV_ARGS(_dxgiDebugInterface.GetAddressOf())));
        createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
    }

    AssertIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(_dxgiFactory.GetAddressOf())));

    _graphicsDiagnostics = options->GraphicsDiagnostics;
}

void Direct3D12FreeGraphicsService()
{
    if (_dxgiDebugInterface)
    {
        AssertIfFailed(_dxgiDebugInterface->ReportLiveObjects(DXGI_DEBUG_ALL, (DXGI_DEBUG_RLO_FLAGS)(DXGI_DEBUG_RLO_IGNORE_INTERNAL | DXGI_DEBUG_RLO_DETAIL)));
    }
}

void Direct3D12GetAvailableGraphicsDevices(GraphicsDeviceInfo* graphicsDevices, int* count)
{
    ComPtr<IDXGIAdapter4> graphicsAdapter;

    for (uint32_t i = 0; DXGI_ERROR_NOT_FOUND != _dxgiFactory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(graphicsAdapter.ReleaseAndGetAddressOf())); i++)
    {
        DXGI_ADAPTER_DESC3 adapterDescription = {};
        AssertIfFailed(graphicsAdapter->GetDesc3(&adapterDescription));
        
        if ((adapterDescription.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE) == 0)
        {
            ComPtr<ID3D12Device> tempDevice;
            D3D12CreateDevice(graphicsAdapter.Get(), D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(tempDevice.GetAddressOf()));

            if (tempDevice == nullptr)
            {
                continue;
            }

            D3D12_FEATURE_DATA_D3D12_OPTIONS deviceOptions = {};
            AssertIfFailed(tempDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &deviceOptions, sizeof(deviceOptions)));

            D3D12_FEATURE_DATA_D3D12_OPTIONS7 deviceOptions7 = {};
            AssertIfFailed(tempDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &deviceOptions7, sizeof(deviceOptions7)));

            D3D12_FEATURE_DATA_SHADER_MODEL shaderModel = {};
            shaderModel.HighestShaderModel = D3D_SHADER_MODEL_6_7;

            AssertIfFailed(tempDevice->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel)));

            if (deviceOptions.ResourceHeapTier == D3D12_RESOURCE_HEAP_TIER_2 && 
                deviceOptions.ResourceBindingTier == D3D12_RESOURCE_BINDING_TIER_3 && 
                deviceOptions7.MeshShaderTier == D3D12_MESH_SHADER_TIER_1 &&
                shaderModel.HighestShaderModel == D3D_SHADER_MODEL_6_7)
            {
                graphicsDevices[(*count)++] = Direct3D12ConstructGraphicsDeviceInfo(adapterDescription);
            }
        }
    }
}

void* Direct3D12CreateGraphicsDevice(GraphicsDeviceOptions* options)
{
    ComPtr<IDXGIAdapter4> graphicsAdapter;
    DXGI_ADAPTER_DESC3 adapterDescription = {};
    bool foundAdapter = false;

    for (uint32_t i = 0; DXGI_ERROR_NOT_FOUND != _dxgiFactory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(graphicsAdapter.GetAddressOf())); i++)
    {
        AssertIfFailed(graphicsAdapter->GetDesc3(&adapterDescription));

        if (Direct3D12GetDeviceId(adapterDescription) == options->DeviceId)
        {
            foundAdapter = true;
            break;
        }
    }

    if (!foundAdapter)
    {
        return nullptr;
    }

    ComPtr<ID3D12Device10> device;
    HRESULT result = D3D12CreateDevice(graphicsAdapter.Get(), D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(device.GetAddressOf()));

    if (FAILED(result))
    {
        return nullptr;
    }

    auto graphicsDevice = new Direct3D12GraphicsDevice();
    graphicsDevice->Device = device;
    graphicsDevice->AdapterDescription = adapterDescription;
    graphicsDevice->InternalId = InterlockedIncrement(&_currentDeviceInternalId) - 1;

    // TODO: Provide an option for this bad but useful feature?
    // If so, it must be with an additional flag
    //AssertIfFailed(this->graphicsDevice->SetStablePowerState(true));
    // TODO: see also https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12device6-setbackgroundprocessingmode
    
    if (_graphicsDiagnostics == GraphicsDiagnostics_Debug)
    {
        graphicsDevice->Device->QueryInterface(IID_PPV_ARGS(_debugInfoQueue.GetAddressOf()));

        if (_debugInfoQueue)
        {
            DWORD callBackCookie = 0;
            AssertIfFailed(_debugInfoQueue->RegisterMessageCallback(DebugReportCallback, D3D12_MESSAGE_CALLBACK_IGNORE_FILTERS, nullptr, &callBackCookie));
        }
    }

    // HACK: We need to have a kind of manager for that with maybe a freeslot list?
    D3D12_DESCRIPTOR_HEAP_DESC rtvDescriptorHeapDesc = {};
    rtvDescriptorHeapDesc.NumDescriptors = 1000; //HACK: Change that
    rtvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    AssertIfFailed(graphicsDevice->Device->CreateDescriptorHeap(&rtvDescriptorHeapDesc, IID_PPV_ARGS(graphicsDevice->RtvDescriptorHeap.ReleaseAndGetAddressOf())));
    graphicsDevice->RtvDescriptorHandleSize = graphicsDevice->Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    graphicsDevice->CurrentRtvDescriptorOffset = 0;

    return graphicsDevice;
}

void Direct3D12FreeGraphicsDevice(void* graphicsDevicePointer)
{
    auto graphicsDevice = (Direct3D12GraphicsDevice*)graphicsDevicePointer;
    graphicsDevice->PipelineStates.EnumerateEntries(Direct3D12DeletePipelineCacheItem);
    delete graphicsDevice;
}

GraphicsDeviceInfo Direct3D12GetGraphicsDeviceInfo(void* graphicsDevicePointer)
{
    auto graphicsDevice = (Direct3D12GraphicsDevice*)graphicsDevicePointer;
    return Direct3D12ConstructGraphicsDeviceInfo(graphicsDevice->AdapterDescription);
}

void* Direct3D12CreateCommandQueue(void* graphicsDevicePointer, CommandQueueType type)
{
    auto graphicsDevice = (Direct3D12GraphicsDevice*)graphicsDevicePointer;

    D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {};
    commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    if (type == CommandQueueType_Compute)
    {
        commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
    }

    else if (type == CommandQueueType_Copy)
    {
        commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
    }

    auto commandQueue = new Direct3D12CommandQueue(graphicsDevice);
    commandQueue->CommandListType = commandQueueDesc.Type;
    commandQueue->FenceValue = 0;

    AssertIfFailed(graphicsDevice->Device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(commandQueue->DeviceObject.GetAddressOf())));
    AssertIfFailed(graphicsDevice->Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(commandQueue->Fence.GetAddressOf())));

    return commandQueue;
}

void Direct3D12FreeCommandQueue(void* commandQueuePointer)
{
    auto commandQueue = (Direct3D12CommandQueue*)commandQueuePointer;
    
    auto fence = Direct3D12CreateCommandQueueFence(commandQueue);
    Direct3D12WaitForFenceOnCpu(fence);

    delete (Direct3D12CommandQueue*)commandQueuePointer;
}

void Direct3D12SetCommandQueueLabel(void* commandQueuePointer, uint8_t* label)
{
    auto commandQueue = (Direct3D12CommandQueue*)commandQueuePointer;
    commandQueue->DeviceObject->SetName(SystemConvertUtf8ToWideChar(label));
}

void* Direct3D12CreateCommandList(void* commandQueuePointer)
{
    auto commandQueue = (Direct3D12CommandQueue*)commandQueuePointer;

    auto commandAllocator = Direct3D12GetCommandAllocator(commandQueue);
    auto commandList = Direct3D12GetCommandList(commandQueue, commandAllocator);

    AssertIfFailed(commandList->DeviceObject->Reset(commandAllocator->Allocator.Get(), nullptr));
    return commandList;
}

void Direct3D12FreeCommandList(void* commandListPointer)
{
    auto commandList = (Direct3D12CommandList*)commandListPointer;

    if (!commandList->IsFromCommandPool)
    {
        delete commandList;
    }
}

void Direct3D12SetCommandListLabel(void* commandListPointer, uint8_t* label)
{
    auto commandList = (Direct3D12CommandList*)commandListPointer;
    commandList->DeviceObject->SetName(SystemConvertUtf8ToWideChar(label));
}

void Direct3D12CommitCommandList(void* commandListPointer)
{
    auto commandList = (Direct3D12CommandList*)commandListPointer;
    AssertIfFailed(commandList->DeviceObject->Close());
}
    
Fence Direct3D12ExecuteCommandLists(void* commandQueuePointer, void** commandLists, int32_t commandListCount, Fence* fencesToWait, int32_t fenceToWaitCount)
{
    auto commandQueue = (Direct3D12CommandQueue*)commandQueuePointer;

    for (int32_t i = 0; i < fenceToWaitCount; i++)
    {
        auto fenceToWait = fencesToWait[i];
        Direct3D12CommandQueue* commandQueueToWait = (Direct3D12CommandQueue*)fenceToWait.CommandQueuePointer;

        AssertIfFailed(commandQueue->DeviceObject->Wait(commandQueueToWait->Fence.Get(), fenceToWait.FenceValue));
    }

    ID3D12CommandList* commandListsToExecute[64];

    for (int32_t i = 0; i < commandListCount; i++)
    {
        commandListsToExecute[i] = ((Direct3D12CommandList*)commandLists[i])->DeviceObject.Get();
    }

    commandQueue->DeviceObject->ExecuteCommandLists((uint32_t)commandListCount, commandListsToExecute);

    auto fence = Direct3D12CreateCommandQueueFence(commandQueue);
    
    for (int32_t i = 0; i < commandListCount; i++)
    {
        auto commandList = (Direct3D12CommandList*)commandLists[i];
        Direct3D12PushFreeCommandList(commandList->CommandQueue, commandList);
        
        if (commandList->IsFromCommandPool)
        {
            Direct3D12UpdateCommandAllocatorFence(commandList, fence.FenceValue);
        }
    }

    return fence;
}
    
void Direct3D12WaitForFenceOnCpu(Fence fence)
{
    auto commandQueueToWait = (Direct3D12CommandQueue*)fence.CommandQueuePointer;
    
    if (_globalFenceEvent == nullptr)
    {
        _globalFenceEvent = CreateEvent(nullptr, false, false, nullptr);
    }

    if (fence.FenceValue > commandQueueToWait->LastCompletedFenceValue) 
    {
        commandQueueToWait->LastCompletedFenceValue = max(commandQueueToWait->LastCompletedFenceValue, commandQueueToWait->Fence->GetCompletedValue());
    }

    if (fence.FenceValue > commandQueueToWait->LastCompletedFenceValue)
    {
        printf("Wait for Fence on CPU...\n");
        commandQueueToWait->Fence->SetEventOnCompletion(fence.FenceValue, _globalFenceEvent);
        WaitForSingleObject(_globalFenceEvent, INFINITE);
    }
}
    
void Direct3D12ResetCommandAllocation(void* graphicsDevicePointer)
{
    auto graphicsDevice = (Direct3D12GraphicsDevice*)graphicsDevicePointer;
    graphicsDevice->CommandAllocationGeneration++;
}

void Direct3D12FreeTexture(void* texturePointer)
{
    auto texture = (Direct3D12Texture*)texturePointer;

    if (!texture->IsPresentTexture)
    {
        delete texture;
    }
}

void* Direct3D12CreateSwapChain(void* commandQueuePointer, void* windowPointer, SwapChainOptions* options)
{
    auto window = (Win32Window*)windowPointer;
    auto commandQueue = (Direct3D12CommandQueue*)commandQueuePointer;
    auto graphicsDevice = commandQueue->GraphicsDevice;

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};

    if (options->Width == 0 || options->Height == 0)
    {
        auto windowRenderSize = Native_GetWindowRenderSize(window);
        swapChainDesc.Width = windowRenderSize.Width;
        swapChainDesc.Height = windowRenderSize.Height;
    }
    else
    {
        swapChainDesc.Width = options->Width;
        swapChainDesc.Height = options->Height;
    }

    // TODO: Support VRR
    swapChainDesc.BufferCount = 3;
    swapChainDesc.Format = (options->Format == SwapChainFormat_HighDynamicRange) ? DXGI_FORMAT_R16G16B16A16_FLOAT : DXGI_FORMAT_B8G8R8A8_UNORM;
    swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
    swapChainDesc.SampleDesc = { 1, 0 };
    swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;

    // TODO: Support Fullscreen exclusive in the future
    DXGI_SWAP_CHAIN_FULLSCREEN_DESC swapChainFullScreenDesc = {};
    swapChainFullScreenDesc.Windowed = true;
    
    auto swapChain = new Direct3D12SwapChain(graphicsDevice);

    AssertIfFailed(_dxgiFactory->CreateSwapChainForHwnd(commandQueue->DeviceObject.Get(), (HWND)window->WindowHandle, &swapChainDesc, &swapChainFullScreenDesc, nullptr, (IDXGISwapChain1**)swapChain->DeviceObject.GetAddressOf()));  
    AssertIfFailed(_dxgiFactory->MakeWindowAssociation((HWND)window->WindowHandle, DXGI_MWA_NO_ALT_ENTER)); 
 
    swapChain->DeviceObject->SetMaximumFrameLatency(options->MaximumFrameLatency);
    swapChain->CommandQueue = commandQueue;
    swapChain->WaitHandle = swapChain->DeviceObject->GetFrameLatencyWaitableObject();
    swapChain->RenderBufferCount = swapChainDesc.BufferCount;
    swapChain->Format = options->Format;
    swapChain->Width = swapChainDesc.Width;
    swapChain->Height = swapChainDesc.Height;

    Direct3D12CreateSwapChainBackBuffers(swapChain);
    return swapChain;
}

void Direct3D12FreeSwapChain(void* swapChainPointer)
{
    auto swapChain = (Direct3D12SwapChain*)swapChainPointer;
    auto commandQueue = swapChain->CommandQueue;

    auto fence = Direct3D12CreateCommandQueueFence(commandQueue);
    Direct3D12WaitForFenceOnCpu(fence);
    
    for (uint32_t i = 0; i < swapChain->RenderBufferCount; i++)
    {
        delete swapChain->RenderBuffers[i];
    }

    delete swapChain;
}
    
void Direct3D12ResizeSwapChain(void* swapChainPointer, int width, int height)
{
    if (width == 0 || height == 0)
    {
        return;
    }

    auto swapChain = (Direct3D12SwapChain*)swapChainPointer;
    swapChain->Width = (uint32_t)width;
    swapChain->Height = (uint32_t)height;

    auto fence = Direct3D12CreateCommandQueueFence(swapChain->CommandQueue);
    Direct3D12WaitForFenceOnCpu(fence);
    
    for (uint32_t i = 0; i < swapChain->RenderBufferCount; i++)
    {
        swapChain->RenderBuffers[i]->DeviceObject.Reset();
        delete swapChain->RenderBuffers[i];

        swapChain->RenderBuffers[i] = nullptr;
    }
    
    AssertIfFailed(swapChain->DeviceObject->ResizeBuffers(swapChain->RenderBufferCount, (uint32_t)width, (uint32_t)height, DXGI_FORMAT_UNKNOWN, DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT));
    Direct3D12CreateSwapChainBackBuffers(swapChain);
}
    
void* Direct3D12GetSwapChainBackBufferTexture(void* swapChainPointer)
{
    auto swapChain = (Direct3D12SwapChain*)swapChainPointer;
    return swapChain->RenderBuffers[swapChain->DeviceObject->GetCurrentBackBufferIndex()];
}

void Direct3D12PresentSwapChain(void* swapChainPointer)
{
    auto swapChain = (Direct3D12SwapChain*)swapChainPointer;
    AssertIfFailed(swapChain->DeviceObject->Present(1, 0));
    
    Direct3D12ResetCommandAllocation(swapChain->GraphicsDevice);
}

void Direct3D12WaitForSwapChainOnCpu(void* swapChainPointer)
{
    auto swapChain = (Direct3D12SwapChain*)swapChainPointer;

    if (WaitForSingleObjectEx(swapChain->WaitHandle, 1000, true) == WAIT_TIMEOUT)
    {
        assert("Wait for SwapChain timeout");
    }
}

void* Direct3D12CreateShader(void* graphicsDevicePointer, ShaderPart* shaderParts, int32_t shaderPartCount)
{
    auto graphicsDevice = (Direct3D12GraphicsDevice*)graphicsDevicePointer;
    auto shader = new Direct3D12Shader(graphicsDevice);

    for (int32_t i = 0; i < shaderPartCount; i++)
    {
        auto shaderPart = shaderParts[i];

        // TODO: Refactor that!
        uint8_t* dataCopy = new uint8_t[shaderPart.DataCount];
        memcpy(dataCopy, shaderPart.DataPointer, shaderPart.DataCount);

        switch (shaderPart.Stage)
        {
        case ShaderStage_AmplificationShader:
            shader->AmplificationShader = Span<uint8_t>((uint8_t*)dataCopy, shaderPart.DataCount);
            break;

        case ShaderStage_MeshShader:
            AssertIfFailed(graphicsDevice->Device->CreateRootSignature(0, shaderPart.DataPointer, shaderPart.DataCount, IID_PPV_ARGS(shader->RootSignature.GetAddressOf())));
            shader->MeshShader = Span<uint8_t>((uint8_t*)dataCopy, shaderPart.DataCount);
            break;

        case ShaderStage_PixelShader:
            shader->PixelShader = Span<uint8_t>((uint8_t*)dataCopy, shaderPart.DataCount);
            break;
        }
    }

    return shader;
}

void Direct3D12FreeShader(void* shaderPointer)
{
    auto shader = (Direct3D12Shader*)shaderPointer;

    if (!shader->AmplificationShader.IsEmpty())
    {
        delete[] shader->AmplificationShader.Pointer;
    }

    if (!shader->MeshShader.IsEmpty())
    {
        delete[] shader->MeshShader.Pointer;
    }
    
    if (!shader->PixelShader.IsEmpty())
    {
        delete[] shader->PixelShader.Pointer;
    }
    
    delete shader;
}

void Direct3D12BeginRenderPass(void* commandListPointer, RenderPassDescriptor* renderPassDescriptor)
{
    auto commandList = (Direct3D12CommandList*)commandListPointer;

    commandList->CurrentRenderPassDescriptor = *renderPassDescriptor;
    commandList->IsRenderPassActive = true;

    D3D12_RENDER_PASS_RENDER_TARGET_DESC renderTargetDescList[4];
    uint32_t renderTargetDescCount = 0;

    if (renderPassDescriptor->RenderTarget0.HasValue)
    {
        Direct3D12InitRenderPassRenderTarget(commandList, &renderTargetDescList[renderTargetDescCount++], &renderPassDescriptor->RenderTarget0.Value);
    }
    
    if (renderPassDescriptor->RenderTarget1.HasValue)
    {
        Direct3D12InitRenderPassRenderTarget(commandList, &renderTargetDescList[renderTargetDescCount++], &renderPassDescriptor->RenderTarget1.Value);
    }
    
    if (renderPassDescriptor->RenderTarget2.HasValue)
    {
        Direct3D12InitRenderPassRenderTarget(commandList, &renderTargetDescList[renderTargetDescCount++], &renderPassDescriptor->RenderTarget2.Value);
    }
    
    if (renderPassDescriptor->RenderTarget3.HasValue)
    {
        Direct3D12InitRenderPassRenderTarget(commandList, &renderTargetDescList[renderTargetDescCount++], &renderPassDescriptor->RenderTarget3.Value);
    }
    
    D3D12_RENDER_PASS_DEPTH_STENCIL_DESC* depthStencilDesc = nullptr;
/*
    D3D12_RENDER_PASS_DEPTH_STENCIL_DESC tmpDepthDesc = {};

    if (renderDescriptor.DepthTexturePointer.HasValue)
    {
        Direct3D12Texture* depthTexture = (Direct3D12Texture*)renderDescriptor.DepthTexturePointer.Value;

        D3D12_CPU_DESCRIPTOR_HANDLE depthDescriptorHeapHandle = {};
        depthDescriptorHeapHandle.ptr = this->globalDsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + depthTexture->TextureDescriptorOffset;

        tmpDepthDesc.cpuDescriptor = depthDescriptorHeapHandle;
        tmpDepthDesc.DepthBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE;

        if (renderDescriptor.DepthBufferOperation == GraphicsDepthBufferOperation::ClearWrite)
        {
            D3D12_DEPTH_STENCIL_VALUE clearValue = {};
            clearValue.Depth = 0.0f;

            tmpDepthDesc.DepthBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
            tmpDepthDesc.DepthBeginningAccess.Clear.ClearValue.Format = texture->ResourceDesc.Format;
            tmpDepthDesc.DepthBeginningAccess.Clear.ClearValue.DepthStencil = clearValue;
        }

        tmpDepthDesc.DepthEndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;

        depthStencilDesc = &tmpDepthDesc;
    }*/

    commandList->DeviceObject->BeginRenderPass(renderTargetDescCount, renderTargetDescList, depthStencilDesc, D3D12_RENDER_PASS_FLAG_NONE);

    // HACK: This is temporary code!
    auto texture = (Direct3D12Texture*)renderPassDescriptor->RenderTarget0.Value.TexturePointer;

    D3D12_VIEWPORT viewport = {};
    viewport.Width = (float)texture->Width;
    viewport.Height = (float)texture->Height;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    commandList->DeviceObject->RSSetViewports(1, &viewport);

    D3D12_RECT scissorRect = {};
    scissorRect.right = (long)texture->Width;
    scissorRect.bottom = (long)texture->Height;
    commandList->DeviceObject->RSSetScissorRects(1, &scissorRect);
}
    
void Direct3D12EndRenderPass(void* commandListPointer)
{
    auto commandList = (Direct3D12CommandList*)commandListPointer;

    commandList->DeviceObject->EndRenderPass();

    if (commandList->CurrentRenderPassDescriptor.RenderTarget0.HasValue)
    {
        auto texture = (Direct3D12Texture*)commandList->CurrentRenderPassDescriptor.RenderTarget0.Value.TexturePointer;

        if (texture->IsPresentTexture)
        {
            D3D12_TEXTURE_BARRIER renderTargetBarrier = {};
            renderTargetBarrier.AccessBefore = D3D12_BARRIER_ACCESS_RENDER_TARGET;
            renderTargetBarrier.AccessAfter = D3D12_BARRIER_ACCESS_COMMON;
            renderTargetBarrier.LayoutBefore = D3D12_BARRIER_LAYOUT_RENDER_TARGET;
            renderTargetBarrier.LayoutAfter = D3D12_BARRIER_LAYOUT_PRESENT;
            renderTargetBarrier.SyncBefore = D3D12_BARRIER_SYNC_RENDER_TARGET;
            renderTargetBarrier.SyncAfter = D3D12_BARRIER_SYNC_ALL;
            renderTargetBarrier.pResource = texture->DeviceObject.Get();

            D3D12_BARRIER_GROUP textureBarriersGroup;
            textureBarriersGroup = {};
            textureBarriersGroup.Type = D3D12_BARRIER_TYPE_TEXTURE;
            textureBarriersGroup.NumBarriers = 1;
            textureBarriersGroup.pTextureBarriers = &renderTargetBarrier;

            commandList->DeviceObject->Barrier(1, &textureBarriersGroup);
        }
    }

    commandList->CurrentRenderPassDescriptor = {};
    commandList->IsRenderPassActive = false;
}

void Direct3D12SetShader(void* commandListPointer, void* shaderPointer)
{
    auto commandList = (Direct3D12CommandList*)commandListPointer;
    auto graphicsDevice = commandList->GraphicsDevice;
    auto shader = (Direct3D12Shader*)shaderPointer;

    if (commandList->IsRenderPassActive)
    {
        // TODO: Hash the parameters
        // TODO: Async compilation with mutlithread support. (Reserve a slot in the cache, and return the pipelinestate cache object)
        // TODO: Have a separate CompileShader function that will launch the async work.
        // TODO: Have a separate GetShaderStatus method
        // TODO: Block for this method, because it means the user wants to use the shader and wants to wait on purpose
        auto renderPassDescriptor = &commandList->CurrentRenderPassDescriptor;
        auto hash = Direct3D12ComputeRenderPipelineStateHash(shader, renderPassDescriptor);

        // TODO: This is not thread-safe!
        // TODO: We should have a kind of GetOrAdd method 
        if (!graphicsDevice->PipelineStates.ContainsKey(hash))
        {
            // TODO: Review allocators
            printf("Create PipelineState for shader %llu...\n", hash);
            auto pipelineStateCacheItem = new PipelineStateCacheItem();
            pipelineStateCacheItem->PipelineState = Direct3D12CreateRenderPipelineState(shader, &commandList->CurrentRenderPassDescriptor);

            graphicsDevice->PipelineStates.Add(hash, pipelineStateCacheItem);
        }

        auto pipelineState = graphicsDevice->PipelineStates[hash]->PipelineState;
        assert(pipelineState != nullptr);

        commandList->DeviceObject->SetPipelineState(pipelineState.Get());
        commandList->DeviceObject->SetGraphicsRootSignature(shader->RootSignature.Get());
    }
}
    
void Direct3D12SetShaderConstants(void* commandListPointer, uint32_t slot, void* constantValues, int32_t constantValueCount)
{
    if (constantValueCount == 0)
    {
        return;
    }

    auto commandList = (Direct3D12CommandList*)commandListPointer;

    if (commandList->IsRenderPassActive)
    {
        commandList->DeviceObject->SetGraphicsRoot32BitConstants(slot, (uint32_t)constantValueCount / 4u, constantValues, 0);
    }
}
    
void Direct3D12DispatchMesh(void* commandListPointer, uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ)
{
    auto commandList = (Direct3D12CommandList*)commandListPointer;

    // TODO: Check if the current shader on the command list is already compiled 
    if (!commandList->IsRenderPassActive)
    {
        return;
    }
        
    commandList->DeviceObject->DispatchMesh(threadGroupCountX, threadGroupCountY, threadGroupCountZ);
}
   
GraphicsDeviceInfo Direct3D12ConstructGraphicsDeviceInfo(DXGI_ADAPTER_DESC3 adapterDescription)
{
    auto result = GraphicsDeviceInfo();
    result.DeviceName = SystemConvertWideCharToUtf8(adapterDescription.Description);
    result.GraphicsApi = GraphicsApi_Direct3D12;
    result.DeviceId = Direct3D12GetDeviceId(adapterDescription);
    result.AvailableMemory = adapterDescription.DedicatedVideoMemory;

    return result;
}

uint64_t Direct3D12GetDeviceId(DXGI_ADAPTER_DESC3 adapterDescription)
{
    return adapterDescription.DeviceId + GraphicsApi_Direct3D12;
}

CommandAllocatorPoolItem* Direct3D12GetCommandAllocator(Direct3D12CommandQueue* commandQueue)
{
    auto graphicsDevice = commandQueue->GraphicsDevice;

    DeviceCommandAllocators& commandAllocatorCache = CommandAllocators[graphicsDevice->InternalId];

    if (commandAllocatorCache.Generation != graphicsDevice->CommandAllocationGeneration)
    {
        commandAllocatorCache.Reset(graphicsDevice->CommandAllocationGeneration);
    }

    auto& commandAllocatorPool = graphicsDevice->DirectCommandAllocatorsPool;
    auto commandAllocatorPoolItemPointer = &commandAllocatorCache.DirectAllocator;
    
    if (commandQueue->CommandListType == D3D12_COMMAND_LIST_TYPE_COPY)
    {
        commandAllocatorPool = graphicsDevice->CopyCommandAllocatorsPool;
        commandAllocatorPoolItemPointer = &commandAllocatorCache.CopyAllocator;
    }
    else if (commandQueue->CommandListType == D3D12_COMMAND_LIST_TYPE_COMPUTE)
    {
        commandAllocatorPool = graphicsDevice->ComputeCommandAllocatorsPool;
        commandAllocatorPoolItemPointer = &commandAllocatorCache.ComputeAllocator;
    }

    if (*commandAllocatorPoolItemPointer == nullptr)
    {
        CommandAllocatorPoolItem* commandAllocatorPoolItem;
        commandAllocatorPool.GetCurrentItemPointerAndMove(&commandAllocatorPoolItem);

        if (commandAllocatorPoolItem->Allocator == nullptr)
        {
            AssertIfFailed(graphicsDevice->Device->CreateCommandAllocator(commandQueue->CommandListType, IID_PPV_ARGS(commandAllocatorPoolItem->Allocator.ReleaseAndGetAddressOf())));
        }
        else
        {
            assert(commandAllocatorPoolItem->IsInUse == false);

            if (commandAllocatorPoolItem->Fence.FenceValue > 0)
            {
                Direct3D12WaitForFenceOnCpu(commandAllocatorPoolItem->Fence);
            }

            AssertIfFailed(commandAllocatorPoolItem->Allocator->Reset());
            commandAllocatorPoolItem->IsInUse = true;
        }

        *commandAllocatorPoolItemPointer = commandAllocatorPoolItem;
    }

    return (*commandAllocatorPoolItemPointer);
}

void Direct3D12UpdateCommandAllocatorFence(Direct3D12CommandList* commandList, uint64_t fenceValue)
{
    auto fence = Fence();
    fence.CommandQueuePointer = commandList->CommandQueue;
    fence.FenceValue = fenceValue;
    
    commandList->CommandAllocatorPoolItem->Fence = fence;
    commandList->CommandAllocatorPoolItem->IsInUse = false;
}

Direct3D12CommandList* Direct3D12GetCommandList(Direct3D12CommandQueue* commandQueue, CommandAllocatorPoolItem* commandAllocatorPoolItem)
{
    CommandListPoolItem* commandListPoolItem;

    if (commandQueue->CommandListType == D3D12_COMMAND_LIST_TYPE_COPY)
    {
        commandQueue->GraphicsDevice->CopyCommandListsPool.GetCurrentItemPointerAndMove(&commandListPoolItem);
    }
    else if (commandQueue->CommandListType == D3D12_COMMAND_LIST_TYPE_COMPUTE)
    {
        commandQueue->GraphicsDevice->ComputeCommandListsPool.GetCurrentItemPointerAndMove(&commandListPoolItem);
    }
    else
    {
        commandQueue->GraphicsDevice->DirectCommandListsPool.GetCurrentItemPointerAndMove(&commandListPoolItem);
    }

    if (commandListPoolItem->CommandList == nullptr)
    {
        auto commandList = new Direct3D12CommandList(commandQueue, commandQueue->GraphicsDevice);
        AssertIfFailed(commandQueue->GraphicsDevice->Device->CreateCommandList1(0, commandQueue->CommandListType, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(commandList->DeviceObject.GetAddressOf())));

        commandList->IsUsed = true;
        commandList->IsFromCommandPool = true;
        commandList->CommandAllocatorPoolItem = commandAllocatorPoolItem;
        commandListPoolItem->CommandList = commandList;
    }
    else
    {
        assert(commandListPoolItem->CommandList);
        auto commandList = (Direct3D12CommandList*)commandListPoolItem->CommandList;

        if (commandList->IsUsed)
        {
            printf("Warning: Not enough command list objects in the pool. Performance may decrease...\n");
            auto commandListResult = new Direct3D12CommandList(commandQueue, commandQueue->GraphicsDevice);
            commandListResult->IsUsed = true;

            AssertIfFailed(commandQueue->GraphicsDevice->Device->CreateCommandList1(0, commandQueue->CommandListType, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(commandListResult->DeviceObject.GetAddressOf())));

            return commandListResult;
        }

        commandList->CommandAllocatorPoolItem = commandAllocatorPoolItem;
        commandList->IsUsed = true;
    }
    
    return (Direct3D12CommandList*)commandListPoolItem->CommandList;
}

void Direct3D12PushFreeCommandList(Direct3D12CommandQueue* commandQueue, Direct3D12CommandList* commandList)
{
    commandList->IsUsed = false;
}
    
Fence Direct3D12CreateCommandQueueFence(Direct3D12CommandQueue* commandQueue)
{
    // TODO: Use std::atomic
    auto fenceValue = InterlockedIncrement(&commandQueue->FenceValue);
    AssertIfFailed(commandQueue->DeviceObject->Signal(commandQueue->Fence.Get(), fenceValue));

    auto fence = Fence();
    fence.CommandQueuePointer = commandQueue;
    fence.FenceValue = fenceValue;

    return fence;
}

void Direct3D12CreateSwapChainBackBuffers(Direct3D12SwapChain* swapChain)
{
    auto graphicsDevice = swapChain->GraphicsDevice;

    // TODO: Review that and refactor!
    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.Format = (swapChain->Format == SwapChainFormat_HighDynamicRange) ? DXGI_FORMAT_R16G16B16A16_FLOAT : DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

    for (uint32_t i = 0; i < swapChain->RenderBufferCount; i++)
    {
        ComPtr<ID3D12Resource> backBuffer;
        AssertIfFailed(swapChain->DeviceObject->GetBuffer(i, IID_PPV_ARGS(backBuffer.ReleaseAndGetAddressOf())));

        wchar_t buff[64] = {};
          swprintf(buff, 64, L"BackBufferRenderTarget%d", i);
        backBuffer->SetName(buff);

        if (swapChain->RenderBuffers[i] == nullptr)
        {
            Direct3D12Texture* backBufferTexture = new Direct3D12Texture(graphicsDevice);
            backBufferTexture->DeviceObject = backBuffer;
            //backBufferTexture->ResourceState = D3D12_RESOURCE_STATE_PRESENT;
            backBufferTexture->IsPresentTexture = true;
            backBufferTexture->Width = swapChain->Width;
            backBufferTexture->Height = swapChain->Height;

            //backBufferTexture->ResourceDesc = CreateTextureResourceDescription(textureFormat, GraphicsTextureUsage::RenderTarget, width, height, 1, 1, 1);

            swapChain->RenderBuffers[i] = backBufferTexture;

            // TODO: Move that code
            auto rtvDescriptorHeapHandle = graphicsDevice->RtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
            rtvDescriptorHeapHandle.ptr += graphicsDevice->CurrentRtvDescriptorOffset;
            backBufferTexture->RtvDescriptor = rtvDescriptorHeapHandle;

            // TODO: Implement that
            //InterlockedAdd((int32_t*)&graphicsDevice->CurrentRtvDescriptorOffset, graphicsDevice->RtvDescriptorHandleSize);
            graphicsDevice->CurrentRtvDescriptorOffset += graphicsDevice->RtvDescriptorHandleSize;
        }
        else
        {
            swapChain->RenderBuffers[i]->DeviceObject = backBuffer;

            // TODO: Update texture desc
        }

        graphicsDevice->Device->CreateRenderTargetView(backBuffer.Get(), &rtvDesc, swapChain->RenderBuffers[i]->RtvDescriptor);
    }
}
    
void Direct3D12InitRenderPassRenderTarget(Direct3D12CommandList* commandList, D3D12_RENDER_PASS_RENDER_TARGET_DESC* renderPassRenderTargetDesc, RenderPassRenderTarget* renderTarget)
{
    auto texture = (Direct3D12Texture*)renderTarget->TexturePointer;

    // TODO: Do this only if not present texture
    // TODO: Put that in an util function
    D3D12_TEXTURE_BARRIER renderTargetBarrier = {};
    renderTargetBarrier.AccessBefore = D3D12_BARRIER_ACCESS_NO_ACCESS;
    renderTargetBarrier.AccessAfter = D3D12_BARRIER_ACCESS_RENDER_TARGET;
    renderTargetBarrier.LayoutBefore = D3D12_BARRIER_LAYOUT_COMMON;
    renderTargetBarrier.LayoutAfter = D3D12_BARRIER_LAYOUT_RENDER_TARGET;
    renderTargetBarrier.SyncBefore = D3D12_BARRIER_SYNC_NONE;
    renderTargetBarrier.SyncAfter = D3D12_BARRIER_SYNC_ALL;
    renderTargetBarrier.pResource = texture->DeviceObject.Get();

    D3D12_BARRIER_GROUP textureBarriersGroup;
    textureBarriersGroup = {};
    textureBarriersGroup.Type = D3D12_BARRIER_TYPE_TEXTURE;
    textureBarriersGroup.NumBarriers = 1;
    textureBarriersGroup.pTextureBarriers = &renderTargetBarrier;

    commandList->DeviceObject->Barrier(1, &textureBarriersGroup);

    *renderPassRenderTargetDesc = {};
    renderPassRenderTargetDesc->cpuDescriptor = texture->RtvDescriptor;

    // TODO: Allow user code to customize
    if (texture->IsPresentTexture)
    {
        renderPassRenderTargetDesc->BeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD;
        renderPassRenderTargetDesc->EndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
    }
    else
    {
        renderPassRenderTargetDesc->BeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE;
        renderPassRenderTargetDesc->EndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
    }

    if (renderTarget->ClearColor.HasValue)
    {
        auto clearColor = renderTarget->ClearColor.Value;

        // TODO: Use Texture format
        renderPassRenderTargetDesc->BeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
        renderPassRenderTargetDesc->BeginningAccess.Clear.ClearValue.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // texture->ResourceDesc.Format;
        renderPassRenderTargetDesc->BeginningAccess.Clear.ClearValue.Color[0] = clearColor.X;
        renderPassRenderTargetDesc->BeginningAccess.Clear.ClearValue.Color[1] = clearColor.Y;
        renderPassRenderTargetDesc->BeginningAccess.Clear.ClearValue.Color[2] = clearColor.Z;
        renderPassRenderTargetDesc->BeginningAccess.Clear.ClearValue.Color[3] = clearColor.W;
    }
}
    
uint64_t Direct3D12ComputeRenderPipelineStateHash(Direct3D12Shader* shader, RenderPassDescriptor* renderPassDescriptor)
{
    // TODO: For the moment the hash of the shader is base on the pointer
    // Maybe we should base it on the hash of each shader parts data? 
    // This would prevent creating duplicate PSO if 2 shaders contains the same parts (it looks like an edge case)
    // but this would add more processing to generate the hash and this function is perf critical

    // TODO: Hash other render pass parameters
    // TODO: Use FarmHash64? https://github.com/TommasoBelluzzo/FastHashes/tree/master

    return (uint64_t)shader;
}
    
ComPtr<ID3D12PipelineState> Direct3D12CreateRenderPipelineState(Direct3D12Shader* shader, RenderPassDescriptor* renderPassDescriptor)
{
    ComPtr<ID3D12PipelineState> pipelineState;

    D3D12_RT_FORMAT_ARRAY renderTargets = {};

    renderTargets.NumRenderTargets = 1;
    renderTargets.RTFormats[0] = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB; // TODO: Fill Correct Back Buffer Format
   
    DXGI_FORMAT depthFormat = DXGI_FORMAT_UNKNOWN;

    /*if (renderPassDescriptor.DepthTexturePointer.HasValue)
    {
        // TODO: Change that
        depthFormat = DXGI_FORMAT_D32_FLOAT;
    }*/

    DXGI_SAMPLE_DESC sampleDesc = {};
    sampleDesc.Count = 1;
    sampleDesc.Quality = 0;

    D3D12_RASTERIZER_DESC rasterizerState = {};
    rasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    rasterizerState.CullMode = D3D12_CULL_MODE_NONE; // D3D12_CULL_MODE_BACK;
    rasterizerState.FrontCounterClockwise = false;
    rasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
    rasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    rasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    rasterizerState.DepthClipEnable = true;
    rasterizerState.MultisampleEnable = false;
    rasterizerState.AntialiasedLineEnable = false;

    // TODO: Check those 2
    rasterizerState.ForcedSampleCount = 0;
    rasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

   /* 
    D3D12_DEPTH_STENCIL_DESC depthStencilState = {};
    
    if (renderPassDescriptor.DepthBufferOperation != GraphicsDepthBufferOperation::DepthNone)
    {
        depthStencilState.DepthEnable = true;
        depthStencilState.StencilEnable = false;

        if (renderPassDescriptor.DepthBufferOperation == GraphicsDepthBufferOperation::ClearWrite ||
            renderPassDescriptor.DepthBufferOperation == GraphicsDepthBufferOperation::Write)
        {
            depthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        }

        if (renderPassDescriptor.DepthBufferOperation == GraphicsDepthBufferOperation::CompareEqual)
        {
            depthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_EQUAL;
        }

        else
        {
            depthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_GREATER;
        }
    }
*/
    D3D12_BLEND_DESC blendState = {};
    blendState.AlphaToCoverageEnable = false;
    blendState.IndependentBlendEnable = false;
/*
    if (renderPassDescriptor.RenderTarget1BlendOperation.HasValue)
    {
        auto blendOperation = renderPassDescriptor.RenderTarget1BlendOperation.Value;
        blendState.RenderTarget[0] = InitBlendState(blendOperation);
    }

    else
    {*/
    blendState.RenderTarget[0] = {
        false,
        false,
        D3D12_BLEND_ONE,
        D3D12_BLEND_ZERO,
        D3D12_BLEND_OP_ADD,
        D3D12_BLEND_ONE,
        D3D12_BLEND_ZERO,
        D3D12_BLEND_OP_ADD,
        D3D12_LOGIC_OP_NOOP,
        D3D12_COLOR_WRITE_ENABLE_ALL,
    }; // InitBlendState(GraphicsBlendOperation::None);
    //}

    D3D12_PIPELINE_STATE_STREAM_DESC psoStream = {};
    GraphicsPso psoDesc = {};
    
    psoDesc.RootSignature = shader->RootSignature.Get();

    if (!shader->AmplificationShader.IsEmpty())
    {
        psoDesc.AS = { shader->AmplificationShader.Pointer, shader->AmplificationShader.Length };
    }

    psoDesc.MS = { shader->MeshShader.Pointer, shader->MeshShader.Length };

    if (!shader->PixelShader.IsEmpty())
    {
        psoDesc.PS = { shader->PixelShader.Pointer, shader->PixelShader.Length };
    }

    psoDesc.RenderTargets = renderTargets;
    psoDesc.SampleDesc = sampleDesc;
    psoDesc.RasterizerState = rasterizerState;
    psoDesc.DepthStencilFormat = depthFormat;
    //psoDesc.DepthStencilState = depthStencilState;
    psoDesc.BlendState = blendState;

    psoStream.SizeInBytes = sizeof(GraphicsPso);
    psoStream.pPipelineStateSubobjectStream = &psoDesc;

    AssertIfFailed(shader->GraphicsDevice->Device->CreatePipelineState(&psoStream, IID_PPV_ARGS(pipelineState.GetAddressOf())));

    return pipelineState;
}

static void DebugReportCallback(D3D12_MESSAGE_CATEGORY Category, D3D12_MESSAGE_SEVERITY Severity, D3D12_MESSAGE_ID ID, LPCSTR pDescription, void* pContext)
{
    if (Severity != D3D12_MESSAGE_SEVERITY_INFO && Severity != D3D12_MESSAGE_SEVERITY_MESSAGE)
    {
        // TODO: Bind that to an elemental callback
        printf("%s\n", pDescription);
    }
}

static void Direct3D12DeletePipelineCacheItem(uint64_t key, void* data)
{
    auto cacheItem = (PipelineStateCacheItem*)data;
    delete cacheItem;
}