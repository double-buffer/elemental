#pragma once

struct MetalTexture : MetalBaseGraphicsObject
{
    MetalTexture(BaseGraphicsService* graphicsService, MetalGraphicsDevice* graphicsDevice) : MetalBaseGraphicsObject(graphicsService, graphicsDevice) 
    {
        IsPresentTexture = false;
    }
    
    NS::SharedPtr<MTL::Texture> DeviceObject;
    bool IsPresentTexture;
};