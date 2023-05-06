#pragma once

struct MetalCommandQueue : MetalBaseGraphicsObject
{
    MetalCommandQueue(BaseGraphicsService* graphicsService, MetalGraphicsDevice* graphicsDevice) : MetalBaseGraphicsObject(graphicsService, graphicsDevice) 
    {
        FenceValue = 0;
    }
    
    NS::SharedPtr<MTL::CommandQueue> DeviceObject;
    NS::SharedPtr<MTL::SharedEvent> Fence;
    std::atomic_uint64_t FenceValue;
};