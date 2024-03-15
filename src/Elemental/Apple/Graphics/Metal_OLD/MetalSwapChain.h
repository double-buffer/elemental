#pragma once

#include "MetalGraphicsDevice.h"
#include "MetalCommandQueue.h"

struct MetalSwapChain
{
    MetalGraphicsDevice* GraphicsDevice;
    NS::SharedPtr<CA::MetalLayer> DeviceObject;
    NS::SharedPtr<CA::MetalDrawable> BackBufferDrawable;
    MetalCommandQueue* CommandQueue;
    
    dispatch_semaphore_t PresentSemaphore;
};
