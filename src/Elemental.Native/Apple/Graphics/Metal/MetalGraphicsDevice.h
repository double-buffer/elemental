#pragma once

struct MetalPipelineStateCacheItem
{
    NS::SharedPtr<MTL::RenderPipelineState> PipelineState;
};

struct MetalGraphicsDevice : BaseGraphicsObject
{
    MetalGraphicsDevice(BaseGraphicsService* graphicsService) : BaseGraphicsObject(graphicsService) 
    {
    }
    
    NS::SharedPtr<MTL::Device> MetalDevice;
    Dictionary<uint64_t, MetalPipelineStateCacheItem> PipelineStates;
};