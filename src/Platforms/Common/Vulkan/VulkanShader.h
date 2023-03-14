#pragma once

struct VulkanShader : VulkanBaseGraphicsObject
{
    VulkanShader(BaseGraphicsService* graphicsService, VulkanGraphicsDevice* graphicsDevice) : VulkanBaseGraphicsObject(graphicsService, graphicsDevice)
    {
    }

    VkShaderModule MeshShader = nullptr;
    VkShaderModule PixelShader = nullptr;
};