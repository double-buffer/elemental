#pragma once

struct VulkanShader : GraphicsObject
{
    VulkanShader(VulkanGraphicsDevice* graphicsDevice)
    {
        GraphicsDevice = graphicsDevice;
        GraphicsApi = GraphicsApi_Vulkan;
    }

    VulkanGraphicsDevice* GraphicsDevice;
    
    VkShaderModule AmplificationShader = nullptr;
    char AmplificationShaderEntryPoint[255];

    VkShaderModule MeshShader = nullptr;
    char MeshShaderEntryPoint[255];

    VkShaderModule PixelShader = nullptr;
    char PixelShaderEntryPoint[255];
    
    VkPipelineLayout PipelineLayout;
};