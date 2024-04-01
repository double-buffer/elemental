#include "DirectX12GraphicsDevice.h"
#include "SystemDataPool.h"
#include "SystemFunctions.h"
#include "SystemLogging.h"
#include "SystemMemory.h"

#define D3D12SDK_VERSION 613
#define D3D12SDK_PATH ".\\"
#define DIRECTX12_MAX_DEVICES 10u
#define DIRECTX12_MAX_RTVS 1024u

struct DirectX12DescriptorHeapStorage
{
    ComPtr<ID3D12DescriptorHeap> DescriptorHeap;
    uint32_t DescriptorHandleSize;
    uint32_t CurrentIndex;
};

MemoryArena DirectX12MemoryArena;
bool DirectX12DebugLayerEnabled = false;
ComPtr<IDXGIFactory6> DxgiFactory; 

SystemDataPool<DirectX12GraphicsDeviceData, DirectX12GraphicsDeviceDataFull> directX12GraphicsDevicePool;

bool directX12DebugInitialized = false;
ComPtr<IDXGIDebug1> dxgiDebugInterface;
ComPtr<ID3D12Debug6> directX12DebugInterface;
ComPtr<ID3D12DeviceFactory> directX12DeviceFactory;

void InitDirectX12()
{
    auto stackMemoryArena = SystemGetStackMemoryArena();

    ComPtr<ID3D12SDKConfiguration1> directX12SdkConfiguration;
    AssertIfFailed(D3D12GetInterface(CLSID_D3D12SDKConfiguration, IID_PPV_ARGS(directX12SdkConfiguration.GetAddressOf())));
    AssertIfFailed(directX12SdkConfiguration->CreateDeviceFactory(D3D12SDK_VERSION, D3D12SDK_PATH, IID_PPV_ARGS(directX12DeviceFactory.GetAddressOf())));

    UINT dxgiCreateFactoryFlags = 0;

    if (DirectX12DebugLayerEnabled)
    {
        auto sdkDllPath = SystemConcatBuffers<char>(stackMemoryArena, SystemGetExecutableFolderPath(stackMemoryArena), "D3D12SDKLayers.dll");
        auto sdkLayerExists = SystemFileExists(sdkDllPath);

        if (sdkLayerExists)
        {
            SystemLogDebugMessage(ElemLogMessageCategory_Graphics, "Init DirectX12 Debug Mode");

            AssertIfFailed(DXGIGetDebugInterface1(0, IID_PPV_ARGS(dxgiDebugInterface.GetAddressOf())));
            dxgiCreateFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;

            AssertIfFailed(directX12DeviceFactory->GetConfigurationInterface(CLSID_D3D12Debug, IID_PPV_ARGS(directX12DebugInterface.GetAddressOf())));

            if (directX12DebugInterface)
            {
                directX12DebugInterface->EnableDebugLayer();
                directX12DebugInterface->SetEnableGPUBasedValidation(true);
                directX12DebugInterface->SetEnableAutoName(true);

                directX12DebugInitialized = true;
            }
        }
        else
        {
            SystemLogWarningMessage(ElemLogMessageCategory_Graphics, "D3D12SDKLayers.dll not found but EnableGraphicsDebugLayer() was called. Debug layer will not be enabled."); 
        }
    }
    else
    {
        SystemLogDebugMessage(ElemLogMessageCategory_Graphics, "Init DirectX12..."); 
    }

    AssertIfFailed(CreateDXGIFactory2(dxgiCreateFactoryFlags, IID_PPV_ARGS(DxgiFactory.GetAddressOf())));
}

DirectX12GraphicsDeviceData* GetDirectX12GraphicsDeviceData(ElemGraphicsDevice graphicsDevice)
{
    return SystemGetDataPoolItem(directX12GraphicsDevicePool, graphicsDevice);
}

DirectX12GraphicsDeviceDataFull* GetDirectX12GraphicsDeviceDataFull(ElemGraphicsDevice graphicsDevice)
{
    return SystemGetDataPoolItemFull(directX12GraphicsDevicePool, graphicsDevice);
}

void InitDirectX12GraphicsDeviceMemory()
{
    if (!DirectX12MemoryArena.Storage)
    {
        DirectX12MemoryArena = SystemAllocateMemoryArena();
        directX12GraphicsDevicePool = SystemCreateDataPool<DirectX12GraphicsDeviceData, DirectX12GraphicsDeviceDataFull>(DirectX12MemoryArena, DIRECTX12_MAX_DEVICES);

        InitDirectX12();
    }
}

void DirectX12DebugReportCallback(D3D12_MESSAGE_CATEGORY category, D3D12_MESSAGE_SEVERITY severity, D3D12_MESSAGE_ID id, LPCSTR description, void* context)
{
    // TODO: Maybe we should remove this because it displays the creation/destroy of resources
    // for now we hide that because we don't have a pool of commandlists
    if (severity == D3D12_MESSAGE_SEVERITY_INFO)
    {
        return;
    }

    auto messageType = ElemLogMessageType_Debug;

    if (severity == D3D12_MESSAGE_SEVERITY_WARNING)
    {
        messageType = ElemLogMessageType_Warning;
    }
    else if(severity == D3D12_MESSAGE_SEVERITY_ERROR || severity == D3D12_MESSAGE_SEVERITY_CORRUPTION)
    {
        messageType = ElemLogMessageType_Error;
    }

    if (strstr(description, "Live ID3D12Device at") && strstr(description, "Refcount: 1"))
    {
        return;
    }

    auto stackMemoryArena = SystemGetStackMemoryArena();
    SystemLogMessage(messageType, ElemLogMessageCategory_Graphics, "%s", description);
}

ElemGraphicsDeviceInfo DirectX12ConstructGraphicsDeviceInfo(DXGI_ADAPTER_DESC3 adapterDescription)
{
    auto stackMemoryArena = SystemGetStackMemoryArena();

    return 
    {
        .DeviceName = SystemConvertWideCharToUtf8(stackMemoryArena, adapterDescription.Description).Pointer,
        .GraphicsApi = ElemGraphicsApi_DirectX12,
        .DeviceId = *(uint64_t *)&adapterDescription.AdapterLuid,
        .AvailableMemory = adapterDescription.DedicatedVideoMemory
    };
}

bool DirectX12CheckGraphicsDeviceCompatibility(ComPtr<IDXGIAdapter4> graphicsAdapter)
{
    DXGI_ADAPTER_DESC3 adapterDescription = {};
    AssertIfFailed(graphicsAdapter->GetDesc3(&adapterDescription));
    
    if ((adapterDescription.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE) == 0)
    {
        ComPtr<ID3D12Device> tempDevice;
        auto result = directX12DeviceFactory->CreateDevice(graphicsAdapter.Get(), D3D_FEATURE_LEVEL_12_2, IID_PPV_ARGS(tempDevice.GetAddressOf()));

        if (tempDevice == nullptr || FAILED(result))
        {
            return false;
        }

        D3D12_FEATURE_DATA_D3D12_OPTIONS deviceOptions = {};
        AssertIfFailed(tempDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &deviceOptions, sizeof(deviceOptions)));

        D3D12_FEATURE_DATA_D3D12_OPTIONS7 deviceOptions7 = {};
        AssertIfFailed(tempDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &deviceOptions7, sizeof(deviceOptions7)));

        D3D12_FEATURE_DATA_D3D12_OPTIONS16 deviceOptions16 = {};
        AssertIfFailed(tempDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS16, &deviceOptions16, sizeof(deviceOptions16)));

        D3D12_FEATURE_DATA_SHADER_MODEL shaderModel = {};
        shaderModel.HighestShaderModel = D3D_SHADER_MODEL_6_8;
        AssertIfFailed(tempDevice->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel)));

        // TODO: Update checks
        if (deviceOptions.ResourceHeapTier == D3D12_RESOURCE_HEAP_TIER_2 && 
            deviceOptions.ResourceBindingTier == D3D12_RESOURCE_BINDING_TIER_3 && 
            deviceOptions7.MeshShaderTier == D3D12_MESH_SHADER_TIER_1 &&
            shaderModel.HighestShaderModel == D3D_SHADER_MODEL_6_8 && 
            deviceOptions16.GPUUploadHeapSupported)
        {
            return true;
        }
    }

    return false;
}

DirectX12DescriptorHeap CreateDirectX12DescriptorHeap(ComPtr<ID3D12Device10> graphicsDevice, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t length)
{
    D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = 
    {
        .Type = type,
        .NumDescriptors = length,
        .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE
    };

    ComPtr<ID3D12DescriptorHeap> descriptorHeap;
    AssertIfFailed(graphicsDevice->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(descriptorHeap.GetAddressOf())));

    auto descriptorStorage = SystemPushStruct<DirectX12DescriptorHeapStorage>(DirectX12MemoryArena);
    descriptorStorage->DescriptorHeap = descriptorHeap;
    descriptorStorage->DescriptorHandleSize = graphicsDevice->GetDescriptorHandleIncrementSize(type);
    descriptorStorage->CurrentIndex = 0;

    return
    {
        .Storage = descriptorStorage
    };
}

void FreeDirectX12DescriptorHeap(DirectX12DescriptorHeap descriptorHeap)
{
    SystemAssert(descriptorHeap.Storage);
    descriptorHeap.Storage->DescriptorHeap.Reset();
}

D3D12_CPU_DESCRIPTOR_HANDLE CreateDirectX12DescriptorHandle(DirectX12DescriptorHeap descriptorHeap)
{            
    SystemAssert(descriptorHeap.Storage);

    // TODO: Free List
    // TODO: Multi threading?

    auto descriptorIndex = descriptorHeap.Storage->CurrentIndex++;

    auto handle = descriptorHeap.Storage->DescriptorHeap->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += descriptorIndex * descriptorHeap.Storage->DescriptorHandleSize;

    return handle;
}

void FreeDirectX12DescriptorHandle(DirectX12DescriptorHeap descriptorHeap, D3D12_CPU_DESCRIPTOR_HANDLE handle)
{
    SystemLogWarningMessage(ElemLogMessageCategory_Graphics, "DirectX12 Free descriptor handle not implemented");
    // TODO: Free List
}

ComPtr<ID3D12RootSignature> CreateDirectX12RootSignature(ComPtr<ID3D12Device10> graphicsDevice)
{
    D3D12_ROOT_PARAMETER1 rootParameters[1];
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    rootParameters[0].Constants.ShaderRegister = 0;
    rootParameters[0].Constants.RegisterSpace = 0;
    rootParameters[0].Constants.Num32BitValues = 16;
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
    rootSignatureDesc.Desc_1_1.NumParameters = 1;
    rootSignatureDesc.Desc_1_1.pParameters = rootParameters;
    rootSignatureDesc.Desc_1_1.NumStaticSamplers = 0;
    rootSignatureDesc.Desc_1_1.pStaticSamplers = nullptr;
    rootSignatureDesc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED;

    ComPtr<ID3DBlob> serializedRootSignature;
    AssertIfFailed(D3D12SerializeVersionedRootSignature(&rootSignatureDesc, serializedRootSignature.GetAddressOf(), nullptr));

    ComPtr<ID3D12RootSignature> rootSignature;
    AssertIfFailed(graphicsDevice->CreateRootSignature(0, serializedRootSignature->GetBufferPointer(), serializedRootSignature->GetBufferSize(), IID_PPV_ARGS(rootSignature.GetAddressOf())));

    return rootSignature;
}

void DirectX12EnableGraphicsDebugLayer()
{
    DirectX12DebugLayerEnabled = true;
}

ElemGraphicsDeviceInfoSpan DirectX12GetAvailableGraphicsDevices()
{
    InitDirectX12GraphicsDeviceMemory();

    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto deviceInfos = SystemPushArray<ElemGraphicsDeviceInfo>(stackMemoryArena, DIRECTX12_MAX_DEVICES);
    auto currentDeviceInfoIndex = 0u;

    ComPtr<IDXGIAdapter4> graphicsAdapter;

    for (uint32_t i = 0; DXGI_ERROR_NOT_FOUND != DxgiFactory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(graphicsAdapter.ReleaseAndGetAddressOf())); i++)
    {
        if (DirectX12CheckGraphicsDeviceCompatibility(graphicsAdapter))
        {
            DXGI_ADAPTER_DESC3 adapterDescription = {};
            AssertIfFailed(graphicsAdapter->GetDesc3(&adapterDescription));

            deviceInfos[currentDeviceInfoIndex++] = DirectX12ConstructGraphicsDeviceInfo(adapterDescription);
        }
    }

    return
    {
        .Items = deviceInfos.Pointer,
        .Length = currentDeviceInfoIndex
    };
}

ElemGraphicsDevice DirectX12CreateGraphicsDevice(const ElemGraphicsDeviceOptions* options)
{
    InitDirectX12GraphicsDeviceMemory();

    ComPtr<IDXGIAdapter4> graphicsAdapter;
    DXGI_ADAPTER_DESC3 adapterDescription = {};
    bool foundAdapter = false;

    for (uint32_t i = 0; DXGI_ERROR_NOT_FOUND != DxgiFactory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(graphicsAdapter.GetAddressOf())); i++)
    {
        if (DirectX12CheckGraphicsDeviceCompatibility(graphicsAdapter))
        {
            AssertIfFailed(graphicsAdapter->GetDesc3(&adapterDescription));

            if ((options != nullptr && options->DeviceId == *(uint64_t *)&adapterDescription.AdapterLuid) || options == nullptr || options->DeviceId == 0)
            {
                foundAdapter = true;
                break;
            }
        }
    }

    SystemAssertReturnNullHandle(foundAdapter);

    ComPtr<ID3D12Device10> device;
    AssertIfFailedReturnNullHandle(directX12DeviceFactory->CreateDevice(graphicsAdapter.Get(), D3D_FEATURE_LEVEL_12_2, IID_PPV_ARGS(device.GetAddressOf())));

    ComPtr<ID3D12InfoQueue1> debugInfoQueue;
    DWORD debugCallBackCookie = 0;

    if (DirectX12DebugLayerEnabled && directX12DebugInitialized)
    {
        AssertIfFailed(device->QueryInterface(IID_PPV_ARGS(debugInfoQueue.GetAddressOf())));

        if (debugInfoQueue)
        {
            AssertIfFailed(debugInfoQueue->RegisterMessageCallback(DirectX12DebugReportCallback, D3D12_MESSAGE_CALLBACK_IGNORE_FILTERS, nullptr, &debugCallBackCookie));
        }
    }

    auto rtvDescriptorHeap = CreateDirectX12DescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, DIRECTX12_MAX_RTVS);
    auto rootSignature = CreateDirectX12RootSignature(device);

    auto handle = SystemAddDataPoolItem(directX12GraphicsDevicePool, {
        .Device = device,
        .RootSignature = rootSignature
    }); 

    SystemAddDataPoolItemFull(directX12GraphicsDevicePool, handle, {
        .AdapterDescription = adapterDescription,
        .DebugInfoQueue = debugInfoQueue,
        .DebugCallBackCookie = debugCallBackCookie,
        .RTVDescriptorHeap = rtvDescriptorHeap
    });

    return handle;
}

void DirectX12FreeGraphicsDevice(ElemGraphicsDevice graphicsDevice)
{
    SystemAssert(graphicsDevice != ELEM_HANDLE_NULL);

    auto graphicsDeviceData = GetDirectX12GraphicsDeviceData(graphicsDevice);
    SystemAssert(graphicsDeviceData);

    auto graphicsDeviceDataFull = GetDirectX12GraphicsDeviceDataFull(graphicsDevice);
    SystemAssert(graphicsDeviceDataFull);

    FreeDirectX12DescriptorHeap(graphicsDeviceDataFull->RTVDescriptorHeap);
    graphicsDeviceData->Device.Reset();
    graphicsDeviceData->RootSignature.Reset();

    SystemRemoveDataPoolItem(directX12GraphicsDevicePool, graphicsDevice);

    if (SystemGetDataPoolItemCount(directX12GraphicsDevicePool) == 0)
    {
        if (directX12DebugInterface)
        {
            directX12DebugInterface.Reset();
            directX12DebugInterface = nullptr;
        }

        if (DxgiFactory)
        {
            DxgiFactory.Reset();
            DxgiFactory = nullptr;
        }

        if (directX12DeviceFactory)
        {
            directX12DeviceFactory.Reset();
            directX12DeviceFactory = nullptr;
        }

        if (dxgiDebugInterface)
        {
            AssertIfFailed(dxgiDebugInterface->ReportLiveObjects(DXGI_DEBUG_ALL, (DXGI_DEBUG_RLO_FLAGS)(DXGI_DEBUG_RLO_IGNORE_INTERNAL | DXGI_DEBUG_RLO_DETAIL)));
    
            if (graphicsDeviceDataFull->DebugInfoQueue)
            {
                graphicsDeviceDataFull->DebugInfoQueue->UnregisterMessageCallback(graphicsDeviceDataFull->DebugCallBackCookie);
                graphicsDeviceDataFull->DebugInfoQueue.Reset();
            }
        }

        SystemFreeMemoryArena(DirectX12MemoryArena);
        DirectX12MemoryArena = {};
        SystemLogDebugMessage(ElemLogMessageCategory_Graphics, "Releasing DirectX12");
    }
}

ElemGraphicsDeviceInfo DirectX12GetGraphicsDeviceInfo(ElemGraphicsDevice graphicsDevice)
{
    SystemAssert(graphicsDevice != ELEM_HANDLE_NULL);

    auto graphicsDeviceDataFull = GetDirectX12GraphicsDeviceDataFull(graphicsDevice);
    SystemAssert(graphicsDeviceDataFull);

    return DirectX12ConstructGraphicsDeviceInfo(graphicsDeviceDataFull->AdapterDescription);
}
