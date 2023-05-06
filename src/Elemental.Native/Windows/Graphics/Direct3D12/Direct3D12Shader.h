#pragma once
#include "PreCompiledHeader.h"
#include "Direct3D12BaseGraphicsObject.h"

struct Direct3D12Shader : Direct3D12BaseGraphicsObject
{
    Direct3D12Shader(BaseGraphicsService* graphicsService, Direct3D12GraphicsDevice* graphicsDevice) : Direct3D12BaseGraphicsObject(graphicsService, graphicsDevice)
    {
    }

    ComPtr<ID3D12RootSignature> RootSignature = nullptr;

    Span<uint8_t> AmplificationShader;
    Span<uint8_t> MeshShader;
    Span<uint8_t> PixelShader;
};

