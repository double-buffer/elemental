#pragma once

#include "Elemental.h"
#include "volk.h"

struct VulkanTextureData
{
    VkImage DeviceObject;
    VkImageView ImageView;
    VkFormat Format;
    bool IsPresentTexture;
    uint32_t Width;
    uint32_t Height;
};

struct VulkanTextureDataFull
{
    ElemGraphicsDevice GraphicsDevice;
};

VulkanTextureData* GetVulkanTextureData(ElemTexture texture);
VulkanTextureDataFull* GetVulkanTextureDataFull(ElemTexture texture);

ElemTexture CreateVulkanTextureFromResource(ElemGraphicsDevice graphicsDevice, VkImage resource, VkFormat format, uint32_t width, uint32_t height, bool isPresentTexture);
DXGI_FORMAT ConvertToVulkanTextureFormat(ElemTextureFormat format);
            
void VulkanFreeTexture(ElemTexture texture);
