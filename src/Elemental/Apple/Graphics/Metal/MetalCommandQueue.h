#pragma once

#include "MetalGraphicsDevice.h"

struct MetalCommandQueue
{
    MetalGraphicsDevice* GraphicsDevice;
    NS::SharedPtr<MTL::CommandQueue> DeviceObject;
    NS::SharedPtr<MTL::SharedEvent> Fence;
    std::atomic_uint64_t FenceValue;
};
