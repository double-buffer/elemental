#pragma once

struct Direct3D12Texture : GraphicsObject
{
    Direct3D12Texture(Direct3D12GraphicsDevice* graphicsDevice)
    {
        GraphicsDevice = graphicsDevice;
        GraphicsApi = GraphicsApi_Direct3D12;
    }

    Direct3D12GraphicsDevice* GraphicsDevice;
    ComPtr<ID3D12Resource> DeviceObject;
    bool IsPresentTexture;
    uint32_t Width = 0;
    uint32_t Height = 0;

    D3D12_CPU_DESCRIPTOR_HANDLE RtvDescriptor;
};