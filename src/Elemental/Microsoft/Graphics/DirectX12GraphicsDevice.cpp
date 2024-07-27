#include "DirectX12GraphicsDevice.h"
#include "SystemDataPool.h"
#include "SystemFunctions.h"
#include "SystemLogging.h"
#include "SystemMemory.h"

#define D3D12SDK_VERSION 614
#define D3D12SDK_PATH ".\\"
#define DIRECTX12_MAX_RTVS 1024u

struct DirectX12DescriptorHeapFreeListItem
{
    uint32_t Next;
};

struct DirectX12DescriptorHeapStorage
{
    ComPtr<ID3D12DescriptorHeap> DescriptorHeap;
    Span<DirectX12DescriptorHeapFreeListItem> Items;
    uint32_t DescriptorHandleSize;
    uint32_t CurrentIndex;
    uint32_t FreeListIndex;
};

MemoryArena DirectX12MemoryArena;
bool DirectX12DebugLayerEnabled = false;
bool directX12DebugGpuValidationEnabled = false;
bool DirectX12DebugBarrierInfoEnabled = false;
ComPtr<IDXGIFactory6> DxgiFactory; 
ComPtr<IDXGIInfoQueue> DxgiInfoQueue;

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

    auto dxgiCreateFactoryFlags = 0u;

    if (DirectX12DebugLayerEnabled)
    {
        auto sdkDllPath = SystemConcatBuffers<char>(stackMemoryArena, SystemGetExecutableFolderPath(stackMemoryArena), "D3D12SDKLayers.dll");
        auto sdkLayerExists = SystemFileExists(sdkDllPath);

        if (sdkLayerExists)
        {
            SystemLogDebugMessage(ElemLogMessageCategory_Graphics, "Init DirectX12 Debug Mode.");

            AssertIfFailed(DXGIGetDebugInterface1(0, IID_PPV_ARGS(dxgiDebugInterface.GetAddressOf())));
            AssertIfFailed(DXGIGetDebugInterface1(0, IID_PPV_ARGS(DxgiInfoQueue.GetAddressOf())));
            dxgiCreateFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;

            AssertIfFailed(directX12DeviceFactory->GetConfigurationInterface(CLSID_D3D12Debug, IID_PPV_ARGS(directX12DebugInterface.GetAddressOf())));

            if (directX12DebugInterface)
            {
                directX12DebugInterface->EnableDebugLayer();
                directX12DebugInterface->SetEnableAutoName(true);
                
                if (directX12DebugGpuValidationEnabled)
                {
                    SystemLogDebugMessage(ElemLogMessageCategory_Graphics, "Init DirectX12 GPU validation.");
                    directX12DebugInterface->SetEnableGPUBasedValidation(true);
                }

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

    // HACK: Remove this when DXIL signing is fully open source!
    AssertIfFailed(directX12DeviceFactory->EnableExperimentalFeatures(1, &D3D12ExperimentalShaderModels, nullptr, nullptr));
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

    SystemLogMessage(messageType, ElemLogMessageCategory_Graphics, "%s", description);
}

ElemGraphicsDeviceInfo DirectX12ConstructGraphicsDeviceInfo(MemoryArena memoryArena, DXGI_ADAPTER_DESC3 adapterDescription)
{
    return 
    {
        .DeviceName = SystemConvertWideCharToUtf8(memoryArena, adapterDescription.Description).Pointer,
        .GraphicsApi = ElemGraphicsApi_DirectX12,
        .DeviceId = *(uint64_t *)&adapterDescription.AdapterLuid,
        .AvailableMemory = adapterDescription.DedicatedVideoMemory
    };
}

bool DirectX12CheckGraphicsDeviceCompatibility(ComPtr<ID3D12Device10> graphicsDevice, DXGI_ADAPTER_DESC3 adapterDescription)
{
    SystemAssert(graphicsDevice);
    
    if ((adapterDescription.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE) == 0)
    {
        D3D12_FEATURE_DATA_D3D12_OPTIONS deviceOptions = {};
        AssertIfFailed(graphicsDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &deviceOptions, sizeof(deviceOptions)));

        D3D12_FEATURE_DATA_D3D12_OPTIONS7 deviceOptions7 = {};
        AssertIfFailed(graphicsDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &deviceOptions7, sizeof(deviceOptions7)));

        D3D12_FEATURE_DATA_D3D12_OPTIONS16 deviceOptions16 = {};
        AssertIfFailed(graphicsDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS16, &deviceOptions16, sizeof(deviceOptions16)));

        D3D12_FEATURE_DATA_SHADER_MODEL shaderModel = {};
        shaderModel.HighestShaderModel = D3D_SHADER_MODEL_6_8;
        AssertIfFailed(graphicsDevice->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel)));

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

DirectX12DescriptorHeap CreateDirectX12DescriptorHeap(ComPtr<ID3D12Device10> graphicsDevice, MemoryArena memoryArena, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags, uint32_t length)
{
    D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = 
    {
        .Type = type,
        .NumDescriptors = length,
        .Flags = flags
    };

    ComPtr<ID3D12DescriptorHeap> descriptorHeap;
    AssertIfFailed(graphicsDevice->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(descriptorHeap.GetAddressOf())));

    auto descriptorStorage = SystemPushStruct<DirectX12DescriptorHeapStorage>(memoryArena);
    descriptorStorage->DescriptorHeap = descriptorHeap;
    descriptorStorage->Items = SystemPushArray<DirectX12DescriptorHeapFreeListItem>(memoryArena, length);
    descriptorStorage->DescriptorHandleSize = graphicsDevice->GetDescriptorHandleIncrementSize(type);
    descriptorStorage->CurrentIndex = 0;
    descriptorStorage->FreeListIndex = UINT32_MAX;

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

    auto storage = descriptorHeap.Storage;
    auto descriptorIndex = UINT32_MAX;

    do
    {
        if (storage->FreeListIndex == UINT32_MAX)
        {
            descriptorIndex = UINT32_MAX;
            break;
        }
        
        descriptorIndex = storage->FreeListIndex;
    } while (!SystemAtomicCompareExchange(storage->FreeListIndex, descriptorIndex, storage->Items[storage->FreeListIndex].Next));

    if (descriptorIndex == UINT32_MAX)
    {
        descriptorIndex = SystemAtomicAdd(storage->CurrentIndex, 1);
    }

    auto handle = descriptorHeap.Storage->DescriptorHeap->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += descriptorIndex * descriptorHeap.Storage->DescriptorHandleSize;

    return handle;
}

void FreeDirectX12DescriptorHandle(DirectX12DescriptorHeap descriptorHeap, D3D12_CPU_DESCRIPTOR_HANDLE handle)
{
    auto storage = descriptorHeap.Storage;
    auto heapStart = storage->DescriptorHeap->GetCPUDescriptorHandleForHeapStart();
    auto indexToDelete = (handle.ptr - heapStart.ptr) / storage->DescriptorHandleSize;
    
    do
    {
        storage->Items[indexToDelete].Next = storage->FreeListIndex;
    } while (!SystemAtomicCompareExchange(storage->FreeListIndex, storage->FreeListIndex, indexToDelete));
}

uint32_t ConvertDirectX12DescriptorHandleToIndex(DirectX12DescriptorHeap descriptorHeap, D3D12_CPU_DESCRIPTOR_HANDLE handle)
{
    auto storage = descriptorHeap.Storage;
    auto heapStart = storage->DescriptorHeap->GetCPUDescriptorHandleForHeapStart();

    return (handle.ptr - heapStart.ptr) / storage->DescriptorHandleSize;
}

D3D12_CPU_DESCRIPTOR_HANDLE ConvertDirectX12DescriptorIndexToHandle(DirectX12DescriptorHeap descriptorHeap, uint32_t index)
{
    auto handle = descriptorHeap.Storage->DescriptorHeap->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += index * descriptorHeap.Storage->DescriptorHandleSize;

    return handle;
}

ComPtr<ID3D12RootSignature> CreateDirectX12RootSignature(ComPtr<ID3D12Device10> graphicsDevice)
{
    D3D12_ROOT_PARAMETER1 rootParameters[1];
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    rootParameters[0].Constants.ShaderRegister = 0;
    rootParameters[0].Constants.RegisterSpace = 0;
    rootParameters[0].Constants.Num32BitValues = 24;
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

void DirectX12SetGraphicsOptions(const ElemGraphicsOptions* options)
{
    SystemAssert(options);

    if (options->EnableDebugLayer)
    {
        DirectX12DebugLayerEnabled = options->EnableDebugLayer;
    }

    if (options->EnableGpuValidation)
    {
        directX12DebugGpuValidationEnabled = options->EnableGpuValidation;
    }

    if (options->EnableDebugBarrierInfo)
    {
        DirectX12DebugBarrierInfoEnabled = options->EnableDebugBarrierInfo;
    }
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
        DXGI_ADAPTER_DESC3 adapterDescription = {};
        AssertIfFailed(graphicsAdapter->GetDesc3(&adapterDescription));

        ComPtr<ID3D12Device10> device;
        auto result = directX12DeviceFactory->CreateDevice(graphicsAdapter.Get(), D3D_FEATURE_LEVEL_12_2, IID_PPV_ARGS(device.GetAddressOf()));

        if (device == nullptr || FAILED(result))
        {
            continue;
        }

        if (DirectX12CheckGraphicsDeviceCompatibility(device, adapterDescription))
        {
            deviceInfos[currentDeviceInfoIndex++] = DirectX12ConstructGraphicsDeviceInfo(stackMemoryArena, adapterDescription);
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
    ComPtr<ID3D12Device10> device;

    for (uint32_t i = 0; DxgiFactory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(graphicsAdapter.GetAddressOf())) != DXGI_ERROR_NOT_FOUND; i++)
    {
        AssertIfFailed(graphicsAdapter->GetDesc3(&adapterDescription));

        ComPtr<ID3D12Device10> candidateDevice;
        auto result = directX12DeviceFactory->CreateDevice(graphicsAdapter.Get(), D3D_FEATURE_LEVEL_12_2, IID_PPV_ARGS(candidateDevice.GetAddressOf()));

        if (candidateDevice == nullptr || FAILED(result))
        {
            continue;
        }

        if (DirectX12CheckGraphicsDeviceCompatibility(candidateDevice, adapterDescription))
        {
            if ((options != nullptr && options->DeviceId == *(uint64_t *)&adapterDescription.AdapterLuid) || options == nullptr || options->DeviceId == 0)
            {
                device = candidateDevice;
                break;
            }
        }
    }

    SystemAssertReturnNullHandle(device);

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

    // TODO: Don't enable it by default
    //AssertIfFailed(device->SetStablePowerState(true));

    auto memoryArena = SystemAllocateMemoryArena();

    auto resourceDescriptorHeap = CreateDirectX12DescriptorHeap(device, memoryArena, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, DIRECTX12_MAX_RESOURCES);
    auto rtvDescriptorHeap = CreateDirectX12DescriptorHeap(device, memoryArena, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, DIRECTX12_MAX_RTVS);
    auto rootSignature = CreateDirectX12RootSignature(device);

    auto handle = SystemAddDataPoolItem(directX12GraphicsDevicePool, {
        .Device = device,
        .RootSignature = rootSignature,
        .ResourceDescriptorHeap = resourceDescriptorHeap,
        .RTVDescriptorHeap = rtvDescriptorHeap,
        .MemoryArena = memoryArena
    }); 

    SystemAddDataPoolItemFull(directX12GraphicsDevicePool, handle, {
        .AdapterDescription = adapterDescription,
        .DebugInfoQueue = debugInfoQueue,
        .DebugCallBackCookie = debugCallBackCookie,
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

    FreeDirectX12DescriptorHeap(graphicsDeviceData->ResourceDescriptorHeap);
    FreeDirectX12DescriptorHeap(graphicsDeviceData->RTVDescriptorHeap);
    graphicsDeviceData->Device.Reset();
    graphicsDeviceData->RootSignature.Reset();

    if (SystemGetDataPoolItemCount(directX12GraphicsDevicePool) == 1)
    {
        SystemLogDebugMessage(ElemLogMessageCategory_Graphics, "Releasing DirectX12");
    }

    if (graphicsDeviceDataFull->DebugInfoQueue)
    {
        if (dxgiDebugInterface)
        {
            AssertIfFailed(dxgiDebugInterface->ReportLiveObjects(DXGI_DEBUG_ALL, (DXGI_DEBUG_RLO_FLAGS)(DXGI_DEBUG_RLO_IGNORE_INTERNAL | DXGI_DEBUG_RLO_DETAIL)));
        }

        graphicsDeviceDataFull->DebugInfoQueue->UnregisterMessageCallback(graphicsDeviceDataFull->DebugCallBackCookie);
        graphicsDeviceDataFull->DebugInfoQueue.Reset();
    }
    
    SystemFreeMemoryArena(graphicsDeviceData->MemoryArena);
    SystemRemoveDataPoolItem(directX12GraphicsDevicePool, graphicsDevice);
}

ElemGraphicsDeviceInfo DirectX12GetGraphicsDeviceInfo(ElemGraphicsDevice graphicsDevice)
{
    SystemAssert(graphicsDevice != ELEM_HANDLE_NULL);

    auto stackMemoryArena = SystemGetStackMemoryArena();

    auto graphicsDeviceDataFull = GetDirectX12GraphicsDeviceDataFull(graphicsDevice);
    SystemAssert(graphicsDeviceDataFull);

    return DirectX12ConstructGraphicsDeviceInfo(stackMemoryArena, graphicsDeviceDataFull->AdapterDescription);
}
