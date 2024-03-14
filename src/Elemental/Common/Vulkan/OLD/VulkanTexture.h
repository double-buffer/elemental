#pragma once

struct VulkanTexture : GraphicsObject
{
    VulkanTexture(VulkanGraphicsDevice* graphicsDevice)
    {
        GraphicsDevice = graphicsDevice;
        GraphicsApi = GraphicsApi_Vulkan;
    }

    VulkanGraphicsDevice* GraphicsDevice;
    VkImage DeviceObject;
    VkImageView ImageView;
    VkFormat Format;
    bool IsPresentTexture = false;
    uint32_t Width;
    uint32_t Height;
};