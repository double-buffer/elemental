#pragma once
#include "WindowsCommon.h"
#include "Direct3D12BaseGraphicsObject.h"

struct Direct3D12Shader : Direct3D12BaseGraphicsObject
{
    Direct3D12Shader(BaseGraphicsService* graphicsService, Direct3D12GraphicsDevice* graphicsDevice) : Direct3D12BaseGraphicsObject(graphicsService, graphicsDevice)
    {
    }

    ComPtr<ID3D12RootSignature> RootSignature = nullptr;

    BinaryContainer AmplificationShader;
    BinaryContainer MeshShader;
    BinaryContainer PixelShader;
};

