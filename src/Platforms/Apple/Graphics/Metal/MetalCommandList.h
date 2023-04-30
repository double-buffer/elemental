#pragma once
#include "PreCompiledHeader.h"

struct MetalShader;

struct MetalCommandList : MetalBaseGraphicsObject
{
    MetalCommandList(BaseGraphicsService* graphicsService, MetalGraphicsDevice* graphicsDevice) : MetalBaseGraphicsObject(graphicsService, graphicsDevice) 
    {
        IsRenderPassActive = false;
    }
    
    NS::SharedPtr<MTL::CommandBuffer> DeviceObject;
    NS::SharedPtr<MTL::CommandEncoder> CommandEncoder;
    
    bool IsRenderPassActive;
    RenderPassDescriptor CurrentRenderPassDescriptor;
    NS::SharedPtr<MTL::RenderPassDescriptor> CurrentMetalRenderPassDescriptor;

    MetalShader* CurrentShader;
    NS::SharedPtr<MTL::RenderPipelineState> CurrentRenderPipelineState;
};