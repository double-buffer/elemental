#pragma once

#include "SystemDictionary.h"

struct MetalPipelineStateCacheItem
{
    NS::SharedPtr<MTL::RenderPipelineState> PipelineState;
};

struct MetalGraphicsDevice
{
    NS::SharedPtr<MTL::Device> MetalDevice;

    // TODO: Chagne uint64 because the hashing can be done in the dictionary add function
    SystemDictionary<uint64_t, MetalPipelineStateCacheItem> PipelineStates;
};
