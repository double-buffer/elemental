#pragma once
#ifdef _WINDOWS
#include "WindowsCommon.h"
#endif
#include "VulkanBaseGraphicsObject.h"

struct VulkanTexture : VulkanBaseGraphicsObject
{
    VulkanTexture(BaseGraphicsService* graphicsService, VulkanGraphicsDevice* graphicsDevice) : VulkanBaseGraphicsObject(graphicsService, graphicsDevice)
    {
    }

    VkImage DeviceObject;
    VkImageView ImageView;
    bool IsPresentTexture;
};