#include "DirectX12GraphicsDevice.h"
#include "SystemDataPool.h"
#include "SystemFunctions.h"
#include "SystemLogging.h"
#include "SystemMemory.h"

#define D3D12SDK_VERSION 613
#define D3D12SDK_PATH ".\\"
#define DIRECT3D12_MAX_DEVICES 10u

MemoryArena DirectX12MemoryArena;
bool DirectX12DebugLayerEnabled = false;

SystemDataPool<DirectX12GraphicsDeviceData, DirectX12GraphicsDeviceDataFull> direct3D12GraphicsDevicePool;

bool direct3D12DebugInitialized = false;
ComPtr<IDXGIDebug1> dxgiDebugInterface;
ComPtr<ID3D12Debug6> direct3D12DebugInterface;
ComPtr<IDXGIFactory6> dxgiFactory; 
ComPtr<ID3D12DeviceFactory> direct3D12DeviceFactory;

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

    auto stackMemoryArena = SystemGetStackMemoryArena();
    SystemLogMessage(messageType, ElemLogMessageCategory_Graphics, "%s", description);
}

void InitDirectX12()
{
    auto stackMemoryArena = SystemGetStackMemoryArena();

    ComPtr<ID3D12SDKConfiguration1> direct3D12SdkConfiguration;
    AssertIfFailed(D3D12GetInterface(CLSID_D3D12SDKConfiguration, IID_PPV_ARGS(direct3D12SdkConfiguration.GetAddressOf())));
    AssertIfFailed(direct3D12SdkConfiguration->CreateDeviceFactory(D3D12SDK_VERSION, D3D12SDK_PATH, IID_PPV_ARGS(direct3D12DeviceFactory.GetAddressOf())));

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

            AssertIfFailed(direct3D12DeviceFactory->GetConfigurationInterface(CLSID_D3D12Debug, IID_PPV_ARGS(direct3D12DebugInterface.GetAddressOf())));

            if (direct3D12DebugInterface)
            {
                direct3D12DebugInterface->EnableDebugLayer();
                direct3D12DebugInterface->SetEnableGPUBasedValidation(true);
                direct3D12DebugInterface->SetEnableAutoName(true);

                direct3D12DebugInitialized = true;
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

    AssertIfFailed(CreateDXGIFactory2(dxgiCreateFactoryFlags, IID_PPV_ARGS(dxgiFactory.GetAddressOf())));
}

void InitDirectX12GraphicsDeviceMemory()
{
    if (!DirectX12MemoryArena.Storage)
    {
        DirectX12MemoryArena = SystemAllocateMemoryArena();
        direct3D12GraphicsDevicePool = SystemCreateDataPool<DirectX12GraphicsDeviceData, DirectX12GraphicsDeviceDataFull>(DirectX12MemoryArena, DIRECT3D12_MAX_DEVICES);

        InitDirectX12();
    }
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
        auto result = direct3D12DeviceFactory->CreateDevice(graphicsAdapter.Get(), D3D_FEATURE_LEVEL_12_2, IID_PPV_ARGS(tempDevice.GetAddressOf()));

        if (tempDevice == nullptr || FAILED(result))
        {
            return false;
        }

        D3D12_FEATURE_DATA_D3D12_OPTIONS deviceOptions = {};
        AssertIfFailed(tempDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &deviceOptions, sizeof(deviceOptions)));

        D3D12_FEATURE_DATA_D3D12_OPTIONS7 deviceOptions7 = {};
        AssertIfFailed(tempDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &deviceOptions7, sizeof(deviceOptions7)));

        D3D12_FEATURE_DATA_SHADER_MODEL shaderModel = {};
        shaderModel.HighestShaderModel = D3D_SHADER_MODEL_6_8;
        AssertIfFailed(tempDevice->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel)));

        if (deviceOptions.ResourceHeapTier == D3D12_RESOURCE_HEAP_TIER_2 && 
            deviceOptions.ResourceBindingTier == D3D12_RESOURCE_BINDING_TIER_3 && 
            deviceOptions7.MeshShaderTier == D3D12_MESH_SHADER_TIER_1 &&
            shaderModel.HighestShaderModel == D3D_SHADER_MODEL_6_8)
        {
            return true;
        }
    }

    return false;
}

DirectX12GraphicsDeviceData* GetDirectX12GraphicsDeviceData(ElemGraphicsDevice graphicsDevice)
{
    return SystemGetDataPoolItem(direct3D12GraphicsDevicePool, graphicsDevice);
}

DirectX12GraphicsDeviceDataFull* GetDirectX12GraphicsDeviceDataFull(ElemGraphicsDevice graphicsDevice)
{
    return SystemGetDataPoolItemFull(direct3D12GraphicsDevicePool, graphicsDevice);
}

void DirectX12EnableGraphicsDebugLayer()
{
    DirectX12DebugLayerEnabled = true;
}

ElemGraphicsDeviceInfoSpan DirectX12GetAvailableGraphicsDevices()
{
    InitDirectX12GraphicsDeviceMemory();

    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto deviceInfos = SystemPushArray<ElemGraphicsDeviceInfo>(stackMemoryArena, DIRECT3D12_MAX_DEVICES);
    auto currentDeviceInfoIndex = 0u;

    ComPtr<IDXGIAdapter4> graphicsAdapter;

    for (uint32_t i = 0; DXGI_ERROR_NOT_FOUND != dxgiFactory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(graphicsAdapter.ReleaseAndGetAddressOf())); i++)
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

    for (uint32_t i = 0; DXGI_ERROR_NOT_FOUND != dxgiFactory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(graphicsAdapter.GetAddressOf())); i++)
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
    AssertIfFailedReturnNullHandle(direct3D12DeviceFactory->CreateDevice(graphicsAdapter.Get(), D3D_FEATURE_LEVEL_12_2, IID_PPV_ARGS(device.GetAddressOf())));

    ComPtr<ID3D12InfoQueue1> debugInfoQueue;

    if (DirectX12DebugLayerEnabled && direct3D12DebugInitialized)
    {
        AssertIfFailed(device->QueryInterface(IID_PPV_ARGS(debugInfoQueue.GetAddressOf())));

        if (debugInfoQueue)
        {
            DWORD callBackCookie = 0;
            AssertIfFailed(debugInfoQueue->RegisterMessageCallback(DirectX12DebugReportCallback, D3D12_MESSAGE_CALLBACK_IGNORE_FILTERS, nullptr, &callBackCookie));
        }
    }

    auto handle = SystemAddDataPoolItem(direct3D12GraphicsDevicePool, {
        .Device = device
    }); 

    SystemAddDataPoolItemFull(direct3D12GraphicsDevicePool, handle, {
        .AdapterDescription = adapterDescription,
        .DebugInfoQueue = debugInfoQueue
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

    graphicsDeviceData->Device.Reset();

    if (graphicsDeviceDataFull->DebugInfoQueue)
    {
        graphicsDeviceDataFull->DebugInfoQueue.Reset();
    }

    SystemRemoveDataPoolItem(direct3D12GraphicsDevicePool, graphicsDevice);

    if (SystemGetDataPoolItemCount(direct3D12GraphicsDevicePool) == 0)
    {
        if (direct3D12DebugInterface)
        {
            direct3D12DebugInterface.Reset();
            direct3D12DebugInterface = nullptr;
        }

        if (dxgiFactory)
        {
            dxgiFactory.Reset();
            dxgiFactory = nullptr;
        }

        if (direct3D12DeviceFactory)
        {
            direct3D12DeviceFactory.Reset();
            direct3D12DeviceFactory = nullptr;
        }

        if (dxgiDebugInterface)
        {
            AssertIfFailed(dxgiDebugInterface->ReportLiveObjects(DXGI_DEBUG_ALL, (DXGI_DEBUG_RLO_FLAGS)(DXGI_DEBUG_RLO_IGNORE_INTERNAL | DXGI_DEBUG_RLO_DETAIL)));
        }

        SystemFreeMemoryArena(DirectX12MemoryArena);
        DirectX12MemoryArena = {};
    }
}

ElemGraphicsDeviceInfo DirectX12GetGraphicsDeviceInfo(ElemGraphicsDevice graphicsDevice)
{
    SystemAssert(graphicsDevice != ELEM_HANDLE_NULL);

    auto graphicsDeviceDataFull = GetDirectX12GraphicsDeviceDataFull(graphicsDevice);
    SystemAssert(graphicsDeviceDataFull);

    return DirectX12ConstructGraphicsDeviceInfo(graphicsDeviceDataFull->AdapterDescription);
}
