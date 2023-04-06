#pragma once
#include "WindowsCommon.h"
#include "Direct3D12BaseGraphicsObject.h"

struct Direct3D12Texture : Direct3D12BaseGraphicsObject
{
    Direct3D12Texture(BaseGraphicsService* graphicsService, Direct3D12GraphicsDevice* graphicsDevice) : Direct3D12BaseGraphicsObject(graphicsService, graphicsDevice)
    {
    }

    ComPtr<ID3D12Resource> DeviceObject;
    bool IsPresentTexture;
    uint32_t Width = 0;
    uint32_t Height = 0;

    D3D12_CPU_DESCRIPTOR_HANDLE RtvDescriptor;
};