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
    VkImageLayout ResourceState = VK_IMAGE_LAYOUT_UNDEFINED;
};