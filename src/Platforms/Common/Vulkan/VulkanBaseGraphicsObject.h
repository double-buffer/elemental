#pragma once

struct VulkanBaseGraphicsObject : BaseGraphicsObject
{
    VulkanBaseGraphicsObject(BaseGraphicsService* graphicsService, VulkanGraphicsDevice* graphicsDevice) : BaseGraphicsObject(graphicsService)
    {
        GraphicsDevice = graphicsDevice;
    }

    VulkanGraphicsDevice* GraphicsDevice;
};