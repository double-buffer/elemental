#pragma once

struct VulkanShader : VulkanBaseGraphicsObject
{
    VulkanShader(BaseGraphicsService* graphicsService, VulkanGraphicsDevice* graphicsDevice) : VulkanBaseGraphicsObject(graphicsService, graphicsDevice)
    {
    }
    
    VkShaderModule AmplificationShader = nullptr;
    char AmplificationShaderEntryPoint[255];

    VkShaderModule MeshShader = nullptr;
    char MeshShaderEntryPoint[255];

    VkShaderModule PixelShader = nullptr;
    char PixelShaderEntryPoint[255];
    
    VkPipelineLayout PipelineLayout;
};