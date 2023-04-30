#pragma once
#include "PreCompiledHeader.h"

struct MetalShader : MetalBaseGraphicsObject
{
    MetalShader(BaseGraphicsService* graphicsService, MetalGraphicsDevice* graphicsDevice) : MetalBaseGraphicsObject(graphicsService, graphicsDevice) 
    {
    }
    
    NS::SharedPtr<MTL::Function> AmplificationShader;
    MTL::Size AmplificationThreadCount;
    NS::SharedPtr<MTL::Function> MeshShader;
    MTL::Size MeshThreadCount;

    NS::SharedPtr<MTL::Function> PixelShader;
};