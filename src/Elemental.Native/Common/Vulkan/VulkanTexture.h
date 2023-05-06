#pragma once

struct VulkanTexture : VulkanBaseGraphicsObject
{
    VulkanTexture(BaseGraphicsService* graphicsService, VulkanGraphicsDevice* graphicsDevice) : VulkanBaseGraphicsObject(graphicsService, graphicsDevice)
    {
    }

    VkImage DeviceObject;
    VkImageView ImageView;
    VkFormat Format;
    bool IsPresentTexture = false;
    uint32_t Width;
    uint32_t Height;
};