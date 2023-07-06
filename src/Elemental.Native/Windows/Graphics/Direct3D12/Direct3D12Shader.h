#pragma once

#include "Elemental.h"
#include "SystemFunctions.h"
#include "GraphicsObject.h"
#include "Direct3D12GraphicsDevice.h"

struct Direct3D12Shader : GraphicsObject
{
    Direct3D12Shader(Direct3D12GraphicsDevice* graphicsDevice)
    {
        GraphicsDevice = graphicsDevice;
        GraphicsApi = GraphicsApi_Direct3D12;
    }

    Direct3D12GraphicsDevice* GraphicsDevice;
    ComPtr<ID3D12RootSignature> RootSignature;

    Span<uint8_t> AmplificationShader;
    Span<uint8_t> MeshShader;
    Span<uint8_t> PixelShader;
};
