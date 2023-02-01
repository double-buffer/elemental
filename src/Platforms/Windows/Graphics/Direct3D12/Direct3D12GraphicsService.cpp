#include "WindowsCommon.h"
#include "Direct3D12GraphicsService.h"

void* Direct3D12GraphicsService::CreateGraphicsDevice(GraphicsDeviceOptions options)
{
    if (_dxgiFactory == nullptr)
    {
        InitSdk(options.GraphicsDiagnostics == GraphicsDiagnostics_Debug);
    }

    auto graphicsDevice = new BaseGraphicsObject(this);
    return graphicsDevice;
}

void Direct3D12GraphicsService::DeleteGraphicsDevice(void* graphicsDevicePointer)
{
}

GraphicsDeviceInfo Direct3D12GraphicsService::GetGraphicsDeviceInfo(void* graphicsDevicePointer)
{
    auto result = GraphicsDeviceInfo();
    result.DeviceName = ConvertWStringToUtf8(L"Windows Device éééé");
    result.GraphicsApiName = ConvertWStringToUtf8(L"API");
    result.DriverVersion = ConvertWStringToUtf8(L"1.0");

    return result;
}

void Direct3D12GraphicsService::InitSdk(bool enableDebugDiagnostics)
{
    printf("Init SDK\n");

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

    if (enableDebugDiagnostics)
    {
        AssertIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(_debugInterface.GetAddressOf())));

        if (_debugInterface)
        {
            _debugInterface->EnableDebugLayer();
        }

        createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
    }

    AssertIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(_dxgiFactory.ReleaseAndGetAddressOf())));
}