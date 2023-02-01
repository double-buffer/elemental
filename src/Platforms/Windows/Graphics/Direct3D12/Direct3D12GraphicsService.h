#pragma once
#include "../../../Common/BaseGraphicsService.h"
#include "../StringConverters.h"

using namespace Microsoft::WRL;

#define D3D12SDKVersion 608
#define D3D12SDKPath u8".\\D3D12\\"

class Direct3D12GraphicsService : BaseGraphicsService
{
public:
    void* CreateGraphicsDevice(GraphicsDiagnostics graphicsDiagnostics) override;
    void DeleteGraphicsDevice(void *graphicsDevicePointer) override;
    GraphicsDeviceInfo GetGraphicsDeviceInfo(void *graphicsDevicePointer) override;

private:
    ComPtr<ID3D12SDKConfiguration> _sdkConfiguration;
    ComPtr<IDXGIFactory4> _dxgiFactory; 
    ComPtr<ID3D12Debug5> _debugInterface;

    void InitSdk(bool enableDebugDiagnostics);
};