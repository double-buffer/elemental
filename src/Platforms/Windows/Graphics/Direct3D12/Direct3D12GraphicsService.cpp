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

        createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
    }

    AssertIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(_dxgiFactory.GetAddressOf())));

    // TODO: Setup debug callback!
}

void Direct3D12GraphicsService::GetAvailableGraphicsDevices(GraphicsDeviceInfo* graphicsDevices, int* count)
{
    ComPtr<IDXGIAdapter4> graphicsAdapter;

    for (int i = 0; DXGI_ERROR_NOT_FOUND != _dxgiFactory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(graphicsAdapter.GetAddressOf())); i++)
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
    
    auto graphicsDevice = new Direct3D12GraphicsDevice(this, device);
    graphicsDevice->AdapterDescription = adapterDescription;

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

	Direct3D12CommandQueue* commandQueue = new Direct3D12CommandQueue(this, graphicsDevice->Device);
    commandQueue->CommandListType = commandQueueDesc.Type;

    AssertIfFailed(graphicsDevice->Device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(commandQueue->DeviceObject.GetAddressOf())));
/*
	ComPtr<ID3D12Fence1> commandQueueFence;
	AssertIfFailed(this->graphicsDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(commandQueueFence.ReleaseAndGetAddressOf())));

	auto commandAllocators = new ComPtr<ID3D12CommandAllocator>[CommandAllocatorsCount];

	// Init command allocators for each frame in flight
	// TODO: For multi threading support we need to allocate on allocator per frame per thread
	for (int i = 0; i < CommandAllocatorsCount; i++)
	{
		ComPtr<ID3D12CommandAllocator> commandAllocator;
		AssertIfFailed(this->graphicsDevice->CreateCommandAllocator(commandQueueDesc.Type, IID_PPV_ARGS(commandAllocator.ReleaseAndGetAddressOf())));
		commandAllocators[i] = commandAllocator;
	}

	Direct3D12CommandQueue* commandQueueStruct = new Direct3D12CommandQueue();
	commandQueueStruct->CommandQueueObject = commandQueue;
	commandQueueStruct->CommandAllocators = commandAllocators;
	commandQueueStruct->Type = commandQueueDesc.Type;
	commandQueueStruct->Fence = commandQueueFence;
	commandQueueStruct->FenceValue = 0;*/

	return commandQueue;
}

void Direct3D12GraphicsService::FreeCommandQueue(void* commandQueuePointer)
{
    delete (Direct3D12CommandQueue*)commandQueuePointer;
}

void Direct3D12GraphicsService::SetCommandQueueLabel(void* commandQueuePointer, uint8_t* label)
{
    Direct3D12CommandQueue* commandQueue = (Direct3D12CommandQueue*)commandQueuePointer;
	commandQueue->DeviceObject->SetName(ConvertUtf8ToWString(label).c_str());
}

void* Direct3D12GraphicsService::CreateCommandList(void *commandQueuePointer)
{
    Direct3D12CommandQueue* commandQueue = (Direct3D12CommandQueue*)commandQueuePointer;

    uint32_t threadId = GetThreadId();
    printf("Thread Id: %u\n", threadId);

    // TODO: We need to be carreful about multi threading here, it shouldn't be a problem because
    // the key is framenumber + threadid

    // TODO: Validate that we have an allocator for the queue for the current frame and thread id combination
    // TODO: If not, try to get an existing free command allocator based on frame number and thread id
    // TODO: If available, reset it
    // TODO: If no command allocator available, create a new one

    // TODO: Try to get an existing free command list
    // TODO: If available, reset it
    // TODO: If not, create a new one

    Direct3D12CommandList* commandList = new Direct3D12CommandList(this, commandQueue->Device);
    commandList->CommandListType = commandQueue->CommandListType;

    AssertIfFailed(commandQueue->Device->CreateCommandList1(0, commandQueue->CommandListType, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(commandList->DeviceObject.GetAddressOf())));

    return commandList;
}

void Direct3D12GraphicsService::FreeCommandList(void* commandListPointer)
{
    delete (Direct3D12CommandList*)commandListPointer;
}

void Direct3D12GraphicsService::SetCommandListLabel(void* commandListPointer, uint8_t* label)
{
    Direct3D12CommandList* commandList = (Direct3D12CommandList*)commandListPointer;
	commandList->DeviceObject->SetName(ConvertUtf8ToWString(label).c_str());
}

void Direct3D12GraphicsService::CommitCommandList(void* commandListPointer)
{
    // TODO: Commit the list and put it in the free list
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