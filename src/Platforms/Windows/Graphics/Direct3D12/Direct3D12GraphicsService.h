#pragma once
#include "WindowsCommon.h"
#include "../../../Common/BaseGraphicsService.h"
#include "../../../Common/BaseGraphicsObject.h"
#include "../../../Common/StringConverters.h"

using namespace Microsoft::WRL;

#define D3D12SDKVersion 608
#define D3D12SDKPath u8".\\D3D12\\"

class Direct3D12GraphicsService : BaseGraphicsService
{
public:
    void GetAvailableGraphicsDevices(GraphicsDeviceInfo* graphicsDevices, int* count) override;
    void* CreateGraphicsDevice(GraphicsDeviceOptions options) override;
    void FreeGraphicsDevice(void *graphicsDevicePointer) override;
    GraphicsDeviceInfo GetGraphicsDeviceInfo(void *graphicsDevicePointer) override;

private:
    ComPtr<ID3D12SDKConfiguration> _sdkConfiguration;
    ComPtr<IDXGIFactory6> _dxgiFactory; 
    ComPtr<ID3D12Debug5> _debugInterface;

    void InitSdk(bool enableDebugDiagnostics);
    GraphicsDeviceInfo ConstructGraphicsDeviceInfo(DXGI_ADAPTER_DESC3 adapterDescription);
};