#pragma once
#include "WindowsCommon.h"
#include "Direct3D12BaseGraphicsObject.h"

struct Direct3D12Shader : Direct3D12BaseGraphicsObject
{
    Direct3D12Shader(BaseGraphicsService* graphicsService, Direct3D12GraphicsDevice* graphicsDevice) : Direct3D12BaseGraphicsObject(graphicsService, graphicsDevice)
    {
    }

    ComPtr<ID3D12RootSignature> RootSignature = nullptr;
    ComPtr<ID3DBlob> MeshShader = nullptr;
    ComPtr<ID3DBlob> PixelShader = nullptr;

    // TODO: To replace with a caching system that handle multiple pipelines base
    // on a hash key
    ComPtr<ID3D12PipelineState> PipelineState = nullptr;
};

