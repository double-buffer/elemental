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
    int32_t Width;
    int32_t Height;
};