#pragma once
#include "WindowsCommon.h"
#include "Direct3D12BaseGraphicsObject.h"

struct Direct3D12GraphicsDevice : Direct3D12BaseGraphicsObject
{
    Direct3D12GraphicsDevice(BaseGraphicsService* graphicsService, ComPtr<ID3D12Device10> device) : Direct3D12BaseGraphicsObject(graphicsService, device)
    {
    }

    DXGI_ADAPTER_DESC3 AdapterDescription;
};