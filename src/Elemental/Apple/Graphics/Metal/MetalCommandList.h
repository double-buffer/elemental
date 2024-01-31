#pragma once

#include "Elemental.h"
#include "MetalGraphicsDevice.h"
#include "MetalShader.h"

struct MetalCommandList
{
    MetalGraphicsDevice* GraphicsDevice;
    NS::SharedPtr<MTL::CommandBuffer> DeviceObject;
    NS::SharedPtr<MTL::CommandEncoder> CommandEncoder;
    
    bool IsRenderPassActive;
    RenderPassDescriptor CurrentRenderPassDescriptor;
    NS::SharedPtr<MTL::RenderPassDescriptor> CurrentMetalRenderPassDescriptor;

    MetalShader* CurrentShader;
    NS::SharedPtr<MTL::RenderPipelineState> CurrentRenderPipelineState;
};
