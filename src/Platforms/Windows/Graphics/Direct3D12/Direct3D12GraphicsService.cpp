#include "WindowsCommon.h"
#include "Direct3D12GraphicsService.h"

Direct3D12GraphicsService::Direct3D12GraphicsService(GraphicsServiceOptions options)
{
    // TODO: For the moment we don't use ID3D12SDKConfiguration1 but we should so we can
    // select the SDK version without setting windows developer mode.
    // See: https://github.com/microsoft/DirectX-Specs/blob/master/d3d/IndependentDevices.md

    AssertIfFailed(D3D12GetInterface(CLSID_D3D12SDKConfiguration, IID_PPV_ARGS(_sdkConfiguration.GetAddressOf())));

    /*ComPtr<ID3D12SDKConfiguration1> sdkConfiguration1;
    AssertIfFailed(sdkConfiguration.As(&sdkConfiguration1));

    ComPtr<ID3D12DeviceFactory> deviceFactory;
    AssertIfFailed(sdkConfiguration1->CreateDeviceFactory(D3D12SDKVersion, D3D12SDKPath, IID_PPV_ARGS(deviceFactory.GetAddressOf())));*/

    AssertIfFailed(_sdkConfiguration->SetSDKVersion(D3D12SDKVersion, D3D12SDKPath));

    UINT createFactoryFlags = 0;

    if (options.GraphicsDiagnostics == GraphicsDiagnostics_Debug)
    {
        AssertIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(_debugInterface.GetAddressOf())));

        if (_debugInterface)
        {
            _debugInterface->EnableDebugLayer();
        }

        AssertIfFailed(DXGIGetDebugInterface1(0, IID_PPV_ARGS(_dxgiDebugInterface.GetAddressOf())));
        createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
    }

    AssertIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(_dxgiFactory.GetAddressOf())));

    _graphicsDiagnostics = options.GraphicsDiagnostics;

    // TODO: Setup debug callback!
}

Direct3D12GraphicsService::~Direct3D12GraphicsService()
{
    if (_dxgiDebugInterface)
    {
        AssertIfFailed(_dxgiDebugInterface->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_SUMMARY));
    }
}

void Direct3D12GraphicsService::GetAvailableGraphicsDevices(GraphicsDeviceInfo* graphicsDevices, int* count)
{
    ComPtr<IDXGIAdapter4> graphicsAdapter;

    for (int i = 0; DXGI_ERROR_NOT_FOUND != _dxgiFactory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(graphicsAdapter.ReleaseAndGetAddressOf())); i++)
    {
        DXGI_ADAPTER_DESC3 adapterDescription;
        AssertIfFailed(graphicsAdapter->GetDesc3(&adapterDescription));
        
        if ((adapterDescription.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0)
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
                graphicsDevices[(*count)++] = ConstructGraphicsDeviceInfo(adapterDescription);
            }
        }
    }
}

void* Direct3D12GraphicsService::CreateGraphicsDevice(GraphicsDeviceOptions options)
{
    ComPtr<IDXGIAdapter4> graphicsAdapter;
    DXGI_ADAPTER_DESC3 adapterDescription;
    bool foundAdapter = false;

    for (int i = 0; DXGI_ERROR_NOT_FOUND != _dxgiFactory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(graphicsAdapter.GetAddressOf())); i++)
    {
        AssertIfFailed(graphicsAdapter->GetDesc3(&adapterDescription));

        if (GetDeviceId(adapterDescription) == options.DeviceId)
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

    // TODO: 
    auto graphicsDevice = new Direct3D12GraphicsDevice(this);
    graphicsDevice->Device = device;
    graphicsDevice->AdapterDescription = adapterDescription;

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

    // TODO: We need to have a kind of manager for that with maybe a freeslot list?
    // TODO: This is temporary!
	D3D12_DESCRIPTOR_HEAP_DESC rtvDescriptorHeapDesc = {};
	rtvDescriptorHeapDesc.NumDescriptors = 1000; //TODO: Change that
	rtvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	AssertIfFailed(graphicsDevice->Device->CreateDescriptorHeap(&rtvDescriptorHeapDesc, IID_PPV_ARGS(graphicsDevice->RtvDescriptorHeap.ReleaseAndGetAddressOf())));
	graphicsDevice->RtvDescriptorHandleSize = graphicsDevice->Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	graphicsDevice->CurrentRtvDescriptorOffset = 0;

    return graphicsDevice;
}

void Direct3D12GraphicsService::FreeGraphicsDevice(void* graphicsDevicePointer)
{
    delete (Direct3D12GraphicsDevice*)graphicsDevicePointer;
}

GraphicsDeviceInfo Direct3D12GraphicsService::GetGraphicsDeviceInfo(void* graphicsDevicePointer)
{
    auto graphicsDevice = (Direct3D12GraphicsDevice*)graphicsDevicePointer;
    return ConstructGraphicsDeviceInfo(graphicsDevice->AdapterDescription);
}

void* Direct3D12GraphicsService::CreateCommandQueue(void* graphicsDevicePointer, CommandQueueType type)
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

	auto commandQueue = new Direct3D12CommandQueue(this, graphicsDevice);
    commandQueue->CommandListType = commandQueueDesc.Type;
    commandQueue->FenceValue = 0;

    AssertIfFailed(graphicsDevice->Device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(commandQueue->DeviceObject.GetAddressOf())));
	AssertIfFailed(graphicsDevice->Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(commandQueue->Fence.GetAddressOf())));

	return commandQueue;
}

void Direct3D12GraphicsService::FreeCommandQueue(void* commandQueuePointer)
{
    auto commandQueue = (Direct3D12CommandQueue *)commandQueuePointer;
    
    auto fence = CreateCommandQueueFence(commandQueue);
    WaitForFenceOnCpu(fence);

    delete (Direct3D12CommandQueue*)commandQueuePointer;
}

void Direct3D12GraphicsService::SetCommandQueueLabel(void* commandQueuePointer, uint8_t* label)
{
    auto commandQueue = (Direct3D12CommandQueue*)commandQueuePointer;
	commandQueue->DeviceObject->SetName(ConvertUtf8ToWString(label).c_str());
}

void* Direct3D12GraphicsService::CreateCommandList(void* commandQueuePointer)
{
    auto commandQueue = (Direct3D12CommandQueue*)commandQueuePointer;

    auto commandAllocator = GetCommandAllocator(commandQueue);
    auto direct3DCommandList = GetCommandList(commandQueue);

    auto commandList = new Direct3D12CommandList(commandQueue, this, commandQueue->GraphicsDevice);
    commandList->DeviceObject = direct3DCommandList;

    AssertIfFailed(commandList->DeviceObject->Reset(commandAllocator.Get(), nullptr));
    return commandList;
}

void Direct3D12GraphicsService::FreeCommandList(void* commandListPointer)
{
    delete (Direct3D12CommandList*)commandListPointer;
}

void Direct3D12GraphicsService::SetCommandListLabel(void* commandListPointer, uint8_t* label)
{
    auto commandList = (Direct3D12CommandList*)commandListPointer;
	commandList->DeviceObject->SetName(ConvertUtf8ToWString(label).c_str());
}

void Direct3D12GraphicsService::CommitCommandList(void* commandListPointer)
{
    auto commandList = (Direct3D12CommandList*)commandListPointer;
    AssertIfFailed(commandList->DeviceObject->Close());

    PushFreeCommandList(commandList->CommandQueue, commandList->DeviceObject);
}
    
Fence Direct3D12GraphicsService::ExecuteCommandLists(void* commandQueuePointer, void** commandLists, int32_t commandListCount, Fence* fencesToWait, int32_t fenceToWaitCount)
{
    // TODO: Review and refactor all static stack alloc here

    auto commandQueue = (Direct3D12CommandQueue*)commandQueuePointer;

	for (int32_t i = 0; i < fenceToWaitCount; i++)
	{
		auto fenceToWait = fencesToWait[i];
		Direct3D12CommandQueue* commandQueueToWait = (Direct3D12CommandQueue*)fenceToWait.CommandQueuePointer;

		AssertIfFailed(commandQueue->DeviceObject->Wait(commandQueueToWait->Fence.Get(), fenceToWait.FenceValue));
	}

    ID3D12CommandList *commandListsToExecute[255];

    for (int32_t i = 0; i < commandListCount; i++)
	{
		commandListsToExecute[i] = ((Direct3D12CommandList*)commandLists[i])->DeviceObject.Get();
	}

	commandQueue->DeviceObject->ExecuteCommandLists(commandListCount, commandListsToExecute);
    auto fence = CreateCommandQueueFence(commandQueue);

    UpdateCommandAllocatorFence(commandQueue);

    return fence;
}
    
void Direct3D12GraphicsService::WaitForFenceOnCpu(Fence fence)
{
    auto commandQueueToWait = (Direct3D12CommandQueue*)fence.CommandQueuePointer;
    
    if (_globalFenceEvent == nullptr)
    {
        _globalFenceEvent = CreateEvent(nullptr, false, false, nullptr);
    }

    printf("Before fence wait\n");

    if (fence.FenceValue > commandQueueToWait->LastCompletedFenceValue) 
    {
        commandQueueToWait->LastCompletedFenceValue = max(commandQueueToWait->LastCompletedFenceValue, commandQueueToWait->Fence->GetCompletedValue());
    }

    if (commandQueueToWait->LastCompletedFenceValue <= fence.FenceValue)
	{
        printf("Waiting for fence...\n");
        commandQueueToWait->Fence->SetEventOnCompletion(fence.FenceValue, _globalFenceEvent);
		WaitForSingleObject(_globalFenceEvent, INFINITE);
	}
    
    printf("AFter fence wait\n");
}

void* Direct3D12GraphicsService::CreateSwapChain(void* windowPointer, void* commandQueuePointer, SwapChainOptions options)
{
    auto window = (Win32Window*)windowPointer;
    auto commandQueue = (Direct3D12CommandQueue*)commandQueuePointer;
    auto graphicsDevice = commandQueue->GraphicsDevice;

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};

    if (options.Width == 0 || options.Height == 0)
    {
        auto windowRenderSize = Native_GetWindowRenderSize(window);
        swapChainDesc.Width = windowRenderSize.Width;
        swapChainDesc.Height = windowRenderSize.Height;
    }
    else
    {
        swapChainDesc.Width = options.Width;
        swapChainDesc.Height = options.Height;
    }

    // TODO: Handle options!
    // TODO: Support VRR
    uint32_t renderBufferCount = 3;

    swapChainDesc.BufferCount = renderBufferCount;
    swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // ConvertTextureFormat(textureFormat, true);
    swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
	swapChainDesc.SampleDesc = { 1, 0 };
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;

	DXGI_SWAP_CHAIN_FULLSCREEN_DESC swapChainFullScreenDesc = {};
	swapChainFullScreenDesc.Windowed = true;
	
	auto swapChain = new Direct3D12SwapChain(this, graphicsDevice);

	AssertIfFailed(_dxgiFactory->CreateSwapChainForHwnd(commandQueue->DeviceObject.Get(), (HWND)window->WindowHandle, &swapChainDesc, &swapChainFullScreenDesc, nullptr, (IDXGISwapChain1**)swapChain->DeviceObject.GetAddressOf()));  
    AssertIfFailed(_dxgiFactory->MakeWindowAssociation((HWND)window->WindowHandle, DXGI_MWA_NO_ALT_ENTER)); 
 
    // TODO: Check that parameter
    // TODO: Support Fullscreen exclusive in the future
    // TODO: Support VRR
	swapChain->DeviceObject->SetMaximumFrameLatency(2);
    swapChain->CommandQueue = commandQueue;
	swapChain->WaitHandle = swapChain->DeviceObject->GetFrameLatencyWaitableObject();
    swapChain->RenderBufferCount = renderBufferCount;

    CreateSwapChainBackBuffers(swapChain);
    return swapChain;
}

void Direct3D12GraphicsService::FreeSwapChain(void* swapChainPointer)
{
    auto swapChain = (Direct3D12SwapChain*)swapChainPointer;
    auto commandQueue = swapChain->CommandQueue;

    auto fence = CreateCommandQueueFence(commandQueue);
    WaitForFenceOnCpu(fence);
    
    for (uint32_t i = 0; i < swapChain->RenderBufferCount; i++)
	{
        delete swapChain->RenderBuffers[i];
    }

    delete swapChain;
}
    
void Direct3D12GraphicsService::ResizeSwapChain(void* swapChainPointer, int width, int height)
{
    if (width == 0 || height == 0)
    {
        return;
    }

    auto swapChain = (Direct3D12SwapChain*)swapChainPointer;

	auto fence = CreateCommandQueueFence(swapChain->CommandQueue);
    WaitForFenceOnCpu(fence);
    
    for (uint32_t i = 0; i < swapChain->RenderBufferCount; i++)
	{
        swapChain->RenderBuffers[i]->DeviceObject.Reset();
    }
	
	D3D12_RESOURCE_DESC backBufferDesc;
	backBufferDesc.Width = width;
	backBufferDesc.Height = height;

	AssertIfFailed(swapChain->DeviceObject->ResizeBuffers(swapChain->RenderBufferCount, width, height, DXGI_FORMAT_UNKNOWN, DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT));
    CreateSwapChainBackBuffers(swapChain);
}
    
void* Direct3D12GraphicsService::GetSwapChainBackBufferTexture(void* swapChainPointer)
{
    auto swapChain = (Direct3D12SwapChain*)swapChainPointer;
    return swapChain->RenderBuffers[swapChain->DeviceObject->GetCurrentBackBufferIndex()];
}

void Direct3D12GraphicsService::PresentSwapChain(void* swapChainPointer)
{
    printf("Present (%d)\n", GetThreadId());
    auto swapChain = (Direct3D12SwapChain*)swapChainPointer;
	AssertIfFailed(swapChain->DeviceObject->Present(1, 0));
    
    auto deviceId = swapChain->GraphicsDevice->AdapterDescription.DeviceId;
    swapChain->GraphicsDevice->CommandAllocationGeneration++;
}

void Direct3D12GraphicsService::WaitForSwapChainOnCpu(void* swapChainPointer)
{
    auto swapChain = (Direct3D12SwapChain*)swapChainPointer;

	if (WaitForSingleObjectEx(swapChain->WaitHandle, 1000, true) == WAIT_TIMEOUT)
	{
		assert("Wait for SwapChain timeout");
	}
}

void Direct3D12GraphicsService::BeginRenderPass(void* commandListPointer, RenderPassDescriptor* renderPassDescriptor)
{
    auto commandList = (Direct3D12CommandList*)commandListPointer;
    auto graphicsDevice = commandList->GraphicsDevice;

    commandList->CurrentRenderPassDescriptor = *renderPassDescriptor;

    D3D12_RENDER_PASS_RENDER_TARGET_DESC renderTargetDescList[8];
    uint32_t renderTargetDescCount = 0;

    if (renderPassDescriptor->RenderTarget0.HasValue)
	{
        Direct3D12Texture* texture = (Direct3D12Texture*)renderPassDescriptor->RenderTarget0.Value.TexturePointer;

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

		D3D12_RENDER_PASS_RENDER_TARGET_DESC* renderTargetDesc = &renderTargetDescList[renderTargetDescCount++];
        *renderTargetDesc = {};
        renderTargetDesc->cpuDescriptor = texture->RtvDescriptor;

        // TODO: Allow user code to customize
        if (texture->IsPresentTexture)
        {
            renderTargetDesc->BeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD;
            renderTargetDesc->EndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
        }
        else
        {
            renderTargetDesc->BeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE;
            renderTargetDesc->EndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
        }

		if (renderPassDescriptor->RenderTarget0.Value.ClearColor.HasValue)
		{
            auto clearColor = renderPassDescriptor->RenderTarget0.Value.ClearColor.Value;

            // TODO: Use Texture format
			renderTargetDesc->BeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
            renderTargetDesc->BeginningAccess.Clear.ClearValue.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // texture->ResourceDesc.Format;
            renderTargetDesc->BeginningAccess.Clear.ClearValue.Color[0] = clearColor.X;
            renderTargetDesc->BeginningAccess.Clear.ClearValue.Color[1] = clearColor.Y;
            renderTargetDesc->BeginningAccess.Clear.ClearValue.Color[2] = clearColor.Z;
            renderTargetDesc->BeginningAccess.Clear.ClearValue.Color[3] = clearColor.W;
        }
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
/*
    D3D12_VIEWPORT viewport = {};
    viewport.Width = (float)texture->ResourceDesc.Width;
    viewport.Height = (float)texture->ResourceDesc.Height;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    commandList->CommandListObject->RSSetViewports(1, &viewport);

    D3D12_RECT scissorRect = {};
    scissorRect.right = (long)texture->ResourceDesc.Width;
    scissorRect.bottom = (long)texture->ResourceDesc.Height;
    commandList->CommandListObject->RSSetScissorRects(1, &scissorRect);*/
}
    
void Direct3D12GraphicsService::EndRenderPass(void* commandListPointer)
{
    auto commandList = (Direct3D12CommandList*)commandListPointer;
    auto graphicsDevice = commandList->GraphicsDevice;

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
}
   
GraphicsDeviceInfo Direct3D12GraphicsService::ConstructGraphicsDeviceInfo(DXGI_ADAPTER_DESC3 adapterDescription)
{
    auto result = GraphicsDeviceInfo();
    result.DeviceName = ConvertWStringToUtf8(adapterDescription.Description);
    result.GraphicsApi = GraphicsApi_Direct3D12;
    result.DeviceId = GetDeviceId(adapterDescription);
    result.AvailableMemory = adapterDescription.DedicatedVideoMemory;

    return result;
}

uint64_t Direct3D12GraphicsService::GetDeviceId(DXGI_ADAPTER_DESC3 adapterDescription)
{
    return adapterDescription.DeviceId + GraphicsApi_Direct3D12;
}

ComPtr<ID3D12CommandAllocator> Direct3D12GraphicsService::GetCommandAllocator(Direct3D12CommandQueue* commandQueue)
{
    auto graphicsDevice = commandQueue->GraphicsDevice;
    auto deviceId = graphicsDevice->AdapterDescription.DeviceId;

    if (CommandAllocators.count(deviceId) == 0)
    {
        CommandAllocators[deviceId] = DeviceCommandAllocators();
    }

    DeviceCommandAllocators& commandAllocatorCache = CommandAllocators[deviceId];

    if (commandAllocatorCache.Generation != graphicsDevice->CommandAllocationGeneration)
    {
        commandAllocatorCache.Reset(graphicsDevice->CommandAllocationGeneration);
    }

    if (commandAllocatorCache.DirectAllocator == nullptr)
    {
        CommandAllocatorPoolItem* commandAllocatorPoolItem;
        graphicsDevice->DirectCommandAllocatorsPool.GetCurrentItemPointerAndMove(&commandAllocatorPoolItem);

        if (commandAllocatorPoolItem->Allocator == nullptr)
        {
            // TODO: Get a new allocator from the pool and reset it if found

            printf("Creating CommandAllocator...(%d)\n", GetThreadId());
            AssertIfFailed(graphicsDevice->Device->CreateCommandAllocator(commandQueue->CommandListType, IID_PPV_ARGS(commandAllocatorPoolItem->Allocator.ReleaseAndGetAddressOf())));

        }
        else
        {
            // TODO: Check fence
            printf("Resetting CommandAllocator...%d(Fence: %d) %d\n", commandAllocatorPoolItem->Allocator.Get(), commandAllocatorPoolItem->Fence.FenceValue, GetThreadId());

            if (commandAllocatorPoolItem->Fence.FenceValue > 0)
            {
                WaitForFenceOnCpu(commandAllocatorPoolItem->Fence);
            }

            AssertIfFailed(commandAllocatorPoolItem->Allocator->Reset());
        }

        // TODO: Do other queue types
        commandAllocatorCache.DirectAllocator = commandAllocatorPoolItem;
    }

    // TODO: Do other types
    return commandAllocatorCache.DirectAllocator->Allocator;
}

void Direct3D12GraphicsService::UpdateCommandAllocatorFence(Direct3D12CommandQueue* commandQueue)
{
    auto graphicsDevice = commandQueue->GraphicsDevice;
    auto deviceId = graphicsDevice->AdapterDescription.DeviceId;

    // TODO: Other queue types

    DeviceCommandAllocators& commandAllocatorCache = CommandAllocators[deviceId];

    assert(commandAllocatorCache.DirectAllocator != nullptr);

    auto fence = Fence();
    fence.CommandQueuePointer = commandQueue;
    fence.FenceValue = commandQueue->FenceValue;

    commandAllocatorCache.DirectAllocator->Fence = fence;
}
    
ComPtr<ID3D12GraphicsCommandList7> Direct3D12GraphicsService::GetCommandList(Direct3D12CommandQueue* commandQueue)
{
    ComPtr<ID3D12GraphicsCommandList7> commandList;

    if (!commandQueue->AvailableCommandLists.GetItem(&commandList))
    {
        printf("Creating CommandList...\n");
        AssertIfFailed(commandQueue->GraphicsDevice->Device->CreateCommandList1(0, commandQueue->CommandListType, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(commandList.GetAddressOf())));

        return commandList;
    }

    return commandList;
}
    
void Direct3D12GraphicsService::PushFreeCommandList(Direct3D12CommandQueue* commandQueue, ComPtr<ID3D12GraphicsCommandList7> commandList)
{
    commandQueue->AvailableCommandLists.Add(commandList);
}
    
Fence Direct3D12GraphicsService::CreateCommandQueueFence(Direct3D12CommandQueue* commandQueue)
{
    InterlockedIncrement(&commandQueue->FenceValue);
	AssertIfFailed(commandQueue->DeviceObject->Signal(commandQueue->Fence.Get(), commandQueue->FenceValue));

    auto fence = Fence();
    fence.CommandQueuePointer = commandQueue;
    fence.FenceValue = commandQueue->FenceValue;

    return fence;
}

void Direct3D12GraphicsService::CreateSwapChainBackBuffers(Direct3D12SwapChain* swapChain)
{
    auto graphicsDevice = swapChain->GraphicsDevice;

    // TODO: Review that and refactor!
    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB; // ConvertTextureFormat(textureFormat);
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
    		Direct3D12Texture* backBufferTexture = new Direct3D12Texture(this, graphicsDevice);
            backBufferTexture->DeviceObject = backBuffer;
            //backBufferTexture->ResourceState = D3D12_RESOURCE_STATE_PRESENT;
            backBufferTexture->IsPresentTexture = true;
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

static void DebugReportCallback(D3D12_MESSAGE_CATEGORY Category, D3D12_MESSAGE_SEVERITY Severity, D3D12_MESSAGE_ID ID, LPCSTR pDescription, void* pContext)
{
    printf("Debug Callback\n");
}