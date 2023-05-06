#pragma once

struct MetalSwapChain : MetalBaseGraphicsObject
{
    MetalSwapChain(BaseGraphicsService* graphicsService, MetalGraphicsDevice* graphicsDevice, uint32_t maxFrameLatency) : MetalBaseGraphicsObject(graphicsService, graphicsDevice)
    {
    }
    
    NS::SharedPtr<CA::MetalLayer> DeviceObject;
    NS::SharedPtr<CA::MetalDrawable> BackBufferDrawable;
    MetalCommandQueue* CommandQueue;
    
    dispatch_semaphore_t PresentSemaphore;
};