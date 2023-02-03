#include "WindowsCommon.h"
#include "Direct3D12GraphicsService.h"

void Direct3D12GraphicsService::GetAvailableGraphicsDevices(GraphicsDeviceInfo* graphicsDevices, int* count)
{
    auto test = GraphicsDeviceInfo();
    test.DeviceName = ConvertWStringToUtf8(L"DirectX!");
    test.GraphicsApiName = ConvertWStringToUtf8(L"Teeeest");
    test.DriverVersion = ConvertWStringToUtf8(L"Teeeest");

    graphicsDevices[(*count)++] = test;
}

void* Direct3D12GraphicsService::CreateGraphicsDevice(GraphicsDeviceOptions options)
{
    if (_dxgiFactory == nullptr)
    {
        InitSdk(options.GraphicsDiagnostics == GraphicsDiagnostics_Debug);
    }

    auto graphicsDevice = new BaseGraphicsObject(this);
    return graphicsDevice;
}

void Direct3D12GraphicsService::FreeGraphicsDevice(void* graphicsDevicePointer)
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

/*
ComPtr<IDXGIAdapter4> Direct3D12GraphicsService::FindGraphicsAdapter(const ComPtr<IDXGIFactory4> dxgiFactory)
{	
    ComPtr<IDXGIAdapter1> dxgiAdapter1;
	ComPtr<IDXGIAdapter4> dxgiAdapter4;

	SIZE_T maxDedicatedVideoMemory = 0;

	for (int i = 0; dxgiFactory->EnumAdapters1(i, dxgiAdapter1.ReleaseAndGetAddressOf()) != DXGI_ERROR_NOT_FOUND; i++)
	{
		DXGI_ADAPTER_DESC1 dxgiAdapterDesc1;
		dxgiAdapter1->GetDesc1(&dxgiAdapterDesc1);

		if ((dxgiAdapterDesc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0)
		{
			ComPtr<ID3D12Device> tempDevice;
			D3D12CreateDevice(dxgiAdapter1.Get(), D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(tempDevice.ReleaseAndGetAddressOf()));

			if (tempDevice != nullptr)
			{
				D3D12_FEATURE_DATA_D3D12_OPTIONS deviceOptions = {};
				AssertIfFailed(tempDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &deviceOptions, sizeof(deviceOptions)));

				D3D12_FEATURE_DATA_D3D12_OPTIONS7 deviceOptions7 = {};
				AssertIfFailed(tempDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &deviceOptions7, sizeof(deviceOptions7)));

				D3D12_FEATURE_DATA_SHADER_MODEL shaderModel = {};
				shaderModel.HighestShaderModel = D3D_SHADER_MODEL_6_6;

				// if (dxgiAdapterDesc1.VendorId != 32902)
				// {
				// 	continue;
				// }

				AssertIfFailed(tempDevice->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel)));

				if (deviceOptions.ResourceHeapTier == D3D12_RESOURCE_HEAP_TIER_2 && 
					deviceOptions.ResourceBindingTier == D3D12_RESOURCE_BINDING_TIER_3 && 
					deviceOptions7.MeshShaderTier == D3D12_MESH_SHADER_TIER_1 &&
					shaderModel.HighestShaderModel == D3D_SHADER_MODEL_6_6 &&
					dxgiAdapterDesc1.DedicatedVideoMemory > maxDedicatedVideoMemory)
				{
					this->adapterName = wstring(dxgiAdapterDesc1.Description) + wstring(L" (DirectX 12.1.") + to_wstring(D3D12_SDK_VERSION) + L")";
					maxDedicatedVideoMemory = dxgiAdapterDesc1.DedicatedVideoMemory;
					dxgiAdapter1.As(&dxgiAdapter4);
				}
			}
		}
	}

	return dxgiAdapter4;
}*/