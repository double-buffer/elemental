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
    ComPtr<ID3D12InfoQueue1> debugInfoQueue;
    ComPtr<IDXGIDebug> dxgiDebug;
    ComPtr<ID3D12Device9> graphicsDevice;

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

    _globalFenceEvent = CreateEvent(nullptr, false, false, nullptr);

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
    
    auto graphicsDevice = new Direct3D12GraphicsDevice(this);
    graphicsDevice->Device = device;
    graphicsDevice->AdapterDescription = adapterDescription;

    // TODO: Move that to swapchain
    graphicsDevice->CurrentFrameNumber = 0;

    // TODO: Provide an option for this bad but useful feature?
    // If so, it must be with an additional flag
	//AssertIfFailed(this->graphicsDevice->SetStablePowerState(true));

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

    auto fence = Fence();
    fence.CommandQueuePointer = commandQueuePointer;
    fence.FenceValue = commandQueue->FenceValue;

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
    auto commandQueue = (Direct3D12CommandQueue*)commandQueuePointer;

	for (int32_t i = 0; i < fenceToWaitCount; i++)
	{
		auto fenceToWait = fencesToWait[i];
		Direct3D12CommandQueue* commandQueueToWait = (Direct3D12CommandQueue*)fenceToWait.CommandQueuePointer;

		AssertIfFailed(commandQueue->DeviceObject->Wait(commandQueueToWait->Fence.Get(), fenceToWait.FenceValue));
	}

    ID3D12CommandList* commandListsToExecute[255];

    for (int32_t i = 0; i < commandListCount; i++)
	{
		commandListsToExecute[i] = ((Direct3D12CommandList*)commandLists[i])->DeviceObject.Get();
	}

	commandQueue->DeviceObject->ExecuteCommandLists(commandListCount, commandListsToExecute);

    auto fenceValue = InterlockedIncrement(&commandQueue->FenceValue);
	AssertIfFailed(commandQueue->DeviceObject->Signal(commandQueue->Fence.Get(), fenceValue));

    auto fence = Fence();
    fence.CommandQueuePointer = commandQueue;
    fence.FenceValue = fenceValue;

    return fence;
}
    
void Direct3D12GraphicsService::WaitForFenceOnCpu(Fence fence)
{
    auto commandQueueToWait = (Direct3D12CommandQueue*)fence.CommandQueuePointer;

	if (commandQueueToWait->Fence->GetCompletedValue() < fence.FenceValue)
	{
		commandQueueToWait->Fence->SetEventOnCompletion(fence.FenceValue, _globalFenceEvent);
		WaitForSingleObject(_globalFenceEvent, INFINITE);
	}
}

void* Direct3D12GraphicsService::CreateSwapChain(void* windowPointer, void* commandQueuePointer, SwapChainOptions options)
{
    auto window = (Win32Window*)windowPointer;
    auto commandQueue = (Direct3D12CommandQueue*)commandQueuePointer;

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
	swapChainDesc.BufferCount = 2;
    swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // ConvertTextureFormat(textureFormat, true);
    swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
	swapChainDesc.SampleDesc = { 1, 0 };
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;

	DXGI_SWAP_CHAIN_FULLSCREEN_DESC swapChainFullScreenDesc = {};
	swapChainFullScreenDesc.Windowed = true;
	
	auto swapChain = new Direct3D12SwapChain(this, commandQueue->GraphicsDevice);

	AssertIfFailed(_dxgiFactory->CreateSwapChainForHwnd(commandQueue->DeviceObject.Get(), (HWND)window->WindowHandle, &swapChainDesc, &swapChainFullScreenDesc, nullptr, (IDXGISwapChain1**)swapChain->DeviceObject.GetAddressOf()));
 
    // TODO: Check that parameter
	swapChain->DeviceObject->SetMaximumFrameLatency(1);

	swapChain->CommandQueue = commandQueue;
	swapChain->WaitHandle = swapChain->DeviceObject->GetFrameLatencyWaitableObject();

    return swapChain;
}

void Direct3D12GraphicsService::FreeSwapChain(void* swapChainPointer)
{
    auto swapChain = (Direct3D12SwapChain*)swapChainPointer;
    auto commandQueue = swapChain->CommandQueue;

    auto fence = Fence();
    fence.CommandQueuePointer = commandQueue;
    fence.FenceValue = commandQueue->FenceValue;

    WaitForFenceOnCpu(fence);
    WaitForSwapChainOnCpu(swapChainPointer);

    /*for (int i = 0; i < RenderBuffersCount; i++)
	{
		DeleteTexture(swapChain->BackBufferTextures[i]);
	}*/

    delete (Direct3D12SwapChain*)swapChainPointer;
}
    
void* Direct3D12GraphicsService::GetSwapChainBackBufferTexture(void* swapChainPointer)
{
    auto swapChain = (Direct3D12SwapChain*)swapChainPointer;
    auto texture = new Direct3D12Texture(this, swapChain->GraphicsDevice);
    // TODO:
    return texture;
}

void Direct3D12GraphicsService::PresentSwapChain(void* swapChainPointer)
{
    auto swapChain = (Direct3D12SwapChain*)swapChainPointer;
	AssertIfFailed(swapChain->DeviceObject->Present(1, 0));
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
    if (renderPassDescriptor->RenderTarget0.ClearColor.HasValue)
    {
        auto clearColor = renderPassDescriptor->RenderTarget0.ClearColor.Value;
        printf("Clear Color: %f %f %f %f\n", clearColor.X, clearColor.Y, clearColor.Z, clearColor.W);
    }
}
    
void Direct3D12GraphicsService::EndRenderPass(void* commandListPointer)
{

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
    // TODO: Try to not take into account the frame number but instead use the fence value of the commandqueue
    // to track if the allocator is free for re-use

    // TODO: The goal is to have a simple fast lookup here and do the heavy work in the present
    // method
    // TODO: We have 4 parameters: ThreadID, Device, FrameIndex and CommandType
    // ThreadID can be avoided by have a local pool for each thread.
    // FrameIndex can be avoided by keeping a list of running allocator and when we call present
    // Check that list to insert the available allocators in the free list of each threads
    // CommandType is just 3 possible values so we can fix the array of have 3 separate lists.

    // TODO: This method needs a lot of optimizations!
    // TODO: Avoid using STD here, for now it is a basic working implementation

    // TODO: For the moment we will support only one swapchain per device
    // TODO: We need to be carreful about multi threading here, it shouldn't be a problem because
    // TODO: For the moment we are bound to present :(

    auto graphicsDevice = commandQueue->GraphicsDevice;

    uint32_t frameIndex = graphicsDevice->CurrentFrameNumber % 2;
    uint32_t threadId = GetThreadId();

    if (graphicsDevice->CommandAllocators.size() <= frameIndex)
    {
        graphicsDevice->CommandAllocators.push_back(std::map<D3D12_COMMAND_LIST_TYPE, std::map<uint32_t, ComPtr<ID3D12CommandAllocator>>>());
    }

    if (graphicsDevice->CommandAllocators[frameIndex].count(commandQueue->CommandListType) == 0)
    {
        graphicsDevice->CommandAllocators[frameIndex][commandQueue->CommandListType] = std::map<uint32_t, ComPtr<ID3D12CommandAllocator>>();
    }

    auto dictionary = &graphicsDevice->CommandAllocators[frameIndex][commandQueue->CommandListType];

    if (dictionary->count(threadId) == 0)
    {
        printf("Creating CommandAllocator...\n");
        ComPtr<ID3D12CommandAllocator> commandAllocator;
		AssertIfFailed(graphicsDevice->Device->CreateCommandAllocator(commandQueue->CommandListType, IID_PPV_ARGS(commandAllocator.ReleaseAndGetAddressOf())));
        (*dictionary)[threadId] = commandAllocator;
    }
    else
    {
        AssertIfFailed((*dictionary)[threadId]->Reset());
    }

    // TODO: Reset the command allocator when present is called
    return (*dictionary)[threadId];
}
    
ComPtr<ID3D12GraphicsCommandList7> Direct3D12GraphicsService::GetCommandList(Direct3D12CommandQueue* commandQueue)
{
    // TODO: IMPORTANT: This must be threadsafe !
    // TODO: Try to get an existing free command list
    // TODO: If available, reset it
    // TODO: If not, create a new one

    if (commandQueue->AvailableCommandLists.empty())
    {
        printf("Creating CommandList...\n");
        ComPtr<ID3D12GraphicsCommandList7> direct3DCommandList;
        AssertIfFailed(commandQueue->GraphicsDevice->Device->CreateCommandList1(0, commandQueue->CommandListType, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(direct3DCommandList.GetAddressOf())));

        return direct3DCommandList;
    }

    auto direct3DCommandList = commandQueue->AvailableCommandLists.top();
    commandQueue->AvailableCommandLists.pop();

    return direct3DCommandList;
}
    
void Direct3D12GraphicsService::PushFreeCommandList(Direct3D12CommandQueue* commandQueue, ComPtr<ID3D12GraphicsCommandList7> commandList)
{
    // TODO: IMPORTANT: This must be threadsafe !
    // TODO: Don't put the list in the queue object
    // TODO: Freelist or ring buffer?
    commandQueue->AvailableCommandLists.push(commandList);
}