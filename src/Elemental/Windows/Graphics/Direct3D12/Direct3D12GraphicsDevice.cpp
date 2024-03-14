#include "Direct3D12GraphicsDevice.h"
#include "SystemDataPool.h"
#include "SystemFunctions.h"
#include "SystemLogging.h"
#include "SystemMemory.h"

#define D3D12SDK_VERSION 613
#define D3D12SDK_PATH ".\\"
#define DIRECT3D12_MAXDEVICES 10

MemoryArena Direct3D12GraphicsMemoryArena;
SystemDataPool<Direct3D12GraphicsDeviceData, Direct3D12GraphicsDeviceDataFull> direct3D12GraphicsDevicePool;

bool direct3D12DebugLayerEnabled = false;
ComPtr<IDXGIDebug1> dxgiDebugInterface;
ComPtr<ID3D12Debug6> direct3D12DebugInterface;
ComPtr<IDXGIFactory6> dxgiFactory; 
ComPtr<ID3D12DeviceFactory> direct3D12DeviceFactory;

// TODO: Use new CreateStateObject for PSO?

void InitDirect3D12()
{
    auto stackMemoryArena = SystemGetStackMemoryArena();

    ComPtr<ID3D12SDKConfiguration1> direct3D12SdkConfiguration;
    AssertIfFailed(D3D12GetInterface(CLSID_D3D12SDKConfiguration, IID_PPV_ARGS(direct3D12SdkConfiguration.GetAddressOf())));
    AssertIfFailed(direct3D12SdkConfiguration->CreateDeviceFactory(D3D12SDK_VERSION, D3D12SDK_PATH, IID_PPV_ARGS(direct3D12DeviceFactory.GetAddressOf())));

    UINT dxgiCreateFactoryFlags = 0;

    if (direct3D12DebugLayerEnabled)
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
                direct3D12DebugInterface->SetEnableSynchronizedCommandQueueValidation(true);
            }
        }
        else
        {
            SystemLogWarningMessage(ElemLogMessageCategory_Graphics, "D3D12SDKLayers.dll not found but EnableGraphicsDebugLayer() was called. Debug layer will not be enabled."); 
        }
    }
    else
    {
        SystemLogDebugMessage(ElemLogMessageCategory_Graphics, "Init Direct3D12..."); 
    }

    AssertIfFailed(CreateDXGIFactory2(dxgiCreateFactoryFlags, IID_PPV_ARGS(dxgiFactory.GetAddressOf())));
}

void Direct3D12DebugReportCallback(D3D12_MESSAGE_CATEGORY category, D3D12_MESSAGE_SEVERITY severity, D3D12_MESSAGE_ID id, LPCSTR description, void* context)
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

void InitDirect3D12GraphicsDeviceMemory()
{
    if (!Direct3D12GraphicsMemoryArena.Storage)
    {
        Direct3D12GraphicsMemoryArena = SystemAllocateMemoryArena();
        direct3D12GraphicsDevicePool = SystemCreateDataPool<Direct3D12GraphicsDeviceData, Direct3D12GraphicsDeviceDataFull>(Direct3D12GraphicsMemoryArena, DIRECT3D12_MAXDEVICES);

        InitDirect3D12();
    }
}

ElemGraphicsDeviceInfo Direct3D12ConstructGraphicsDeviceInfo(DXGI_ADAPTER_DESC3 adapterDescription)
{
    auto stackMemoryArena = SystemGetStackMemoryArena();

    return 
    {
        .DeviceName = SystemConvertWideCharToUtf8(stackMemoryArena, adapterDescription.Description).Pointer,
        .GraphicsApi = ElemGraphicsApi_Direct3D12,
        .DeviceId = *(uint64_t *)&adapterDescription.AdapterLuid,
        .AvailableMemory = adapterDescription.DedicatedVideoMemory
    };
}

Direct3D12GraphicsDeviceData* GetDirect3D12GraphicsDeviceData(ElemGraphicsDevice graphicsDevice)
{
    return SystemGetDataPoolItem(direct3D12GraphicsDevicePool, graphicsDevice);
}

Direct3D12GraphicsDeviceDataFull* GetDirect3D12GraphicsDeviceDataFull(ElemGraphicsDevice graphicsDevice)
{
    return SystemGetDataPoolItemFull(direct3D12GraphicsDevicePool, graphicsDevice);
}

void Direct3D12EnableGraphicsDebugLayer()
{
    direct3D12DebugLayerEnabled = true;
}

ElemGraphicsDeviceInfoList Direct3D12GetAvailableGraphicsDevices()
{
    InitDirect3D12GraphicsDeviceMemory();

    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto deviceInfos = SystemPushArray<ElemGraphicsDeviceInfo>(stackMemoryArena, DIRECT3D12_MAXDEVICES);
    auto currentDeviceInfoIndex = 0u;

    ComPtr<IDXGIAdapter4> graphicsAdapter;

    for (uint32_t i = 0; DXGI_ERROR_NOT_FOUND != dxgiFactory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(graphicsAdapter.ReleaseAndGetAddressOf())); i++)
    {
        DXGI_ADAPTER_DESC3 adapterDescription = {};
        AssertIfFailed(graphicsAdapter->GetDesc3(&adapterDescription));
        
        if ((adapterDescription.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE) == 0)
        {
            ComPtr<ID3D12Device> tempDevice;
            AssertIfFailed(direct3D12DeviceFactory->CreateDevice(graphicsAdapter.Get(), D3D_FEATURE_LEVEL_12_2, IID_PPV_ARGS(tempDevice.GetAddressOf())));

            if (tempDevice == nullptr)
            {
                continue;
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
                deviceInfos[currentDeviceInfoIndex++] = Direct3D12ConstructGraphicsDeviceInfo(adapterDescription);
            }
        }
    }

    return
    {
        .Items = deviceInfos.Pointer,
        .Length = currentDeviceInfoIndex
    };
}

ElemGraphicsDevice Direct3D12CreateGraphicsDevice(const ElemGraphicsDeviceOptions* options)
{
    SystemAssert(options);
    SystemAssert(options->DeviceId);

    ComPtr<IDXGIAdapter4> graphicsAdapter;
    DXGI_ADAPTER_DESC3 adapterDescription = {};
    bool foundAdapter = false;

    for (uint32_t i = 0; DXGI_ERROR_NOT_FOUND != dxgiFactory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(graphicsAdapter.GetAddressOf())); i++)
    {
        AssertIfFailed(graphicsAdapter->GetDesc3(&adapterDescription));

        if (options->DeviceId == *(uint64_t *)&adapterDescription.AdapterLuid)
        {
            foundAdapter = true;
            break;
        }
    }

    SystemAssert(foundAdapter);

    ComPtr<ID3D12Device10> device;
    AssertIfFailed(direct3D12DeviceFactory->CreateDevice(graphicsAdapter.Get(), D3D_FEATURE_LEVEL_12_2, IID_PPV_ARGS(device.GetAddressOf())));

    ComPtr<ID3D12InfoQueue1> debugInfoQueue;

    if (direct3D12DebugLayerEnabled)
    {
        AssertIfFailed(device->QueryInterface(IID_PPV_ARGS(debugInfoQueue.GetAddressOf())));

        if (debugInfoQueue)
        {
            DWORD callBackCookie = 0;
            AssertIfFailed(debugInfoQueue->RegisterMessageCallback(Direct3D12DebugReportCallback, D3D12_MESSAGE_CALLBACK_IGNORE_FILTERS, nullptr, &callBackCookie));
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

void Direct3D12FreeGraphicsDevice(ElemGraphicsDevice graphicsDevice)
{
    auto graphicsDeviceData = GetDirect3D12GraphicsDeviceData(graphicsDevice);
    SystemAssert(graphicsDeviceData);

    auto graphicsDeviceDataFull = GetDirect3D12GraphicsDeviceDataFull(graphicsDevice);
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
    }
}

ElemGraphicsDeviceInfo Direct3D12GetGraphicsDeviceInfo(ElemGraphicsDevice graphicsDevice)
{
    auto graphicsDeviceDataFull = GetDirect3D12GraphicsDeviceDataFull(graphicsDevice);
    SystemAssert(graphicsDeviceDataFull);

    return Direct3D12ConstructGraphicsDeviceInfo(graphicsDeviceDataFull->AdapterDescription);
}
