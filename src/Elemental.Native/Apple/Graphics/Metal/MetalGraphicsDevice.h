#pragma once

#include "Dictionary.h"

struct MetalPipelineStateCacheItem
{
    NS::SharedPtr<MTL::RenderPipelineState> PipelineState;
};

struct MetalGraphicsDevice
{
    NS::SharedPtr<MTL::Device> MetalDevice;
    Dictionary<uint64_t, MetalPipelineStateCacheItem> PipelineStates;
};
