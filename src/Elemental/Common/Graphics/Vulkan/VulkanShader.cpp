#include "VulkanShader.h"
#include "VulkanGraphicsDevice.h"
#include "VulkanCommandList.h"
#include "SystemDataPool.h"
#include "SystemFunctions.h"
#include "SystemMemory.h"

#define VULKAN_MAX_LIBRARIES UINT16_MAX
#define VULKAN_MAX_PIPELINESTATES UINT16_MAX

SystemDataPool<VulkanShaderLibraryData, VulkanShaderLibraryDataFull> vulkanShaderLibraryPool;
SystemDataPool<VulkanPipelineStateData, VulkanPipelineStateDataFull> vulkanPipelineStatePool;

void InitVulkanShaderMemory()
{
    if (!vulkanShaderLibraryPool.Storage)
    {
        vulkanShaderLibraryPool = SystemCreateDataPool<VulkanShaderLibraryData, VulkanShaderLibraryDataFull>(VulkanGraphicsMemoryArena, VULKAN_MAX_LIBRARIES);
        vulkanPipelineStatePool = SystemCreateDataPool<VulkanPipelineStateData, VulkanPipelineStateDataFull>(VulkanGraphicsMemoryArena, VULKAN_MAX_PIPELINESTATES);
    }
}

VulkanShaderLibraryData* GetVulkanShaderLibraryData(ElemShaderLibrary shaderLibrary)
{
    return SystemGetDataPoolItem(vulkanShaderLibraryPool, shaderLibrary);
}

VulkanShaderLibraryDataFull* GetVulkanShaderLibraryDataFull(ElemShaderLibrary shaderLibrary)
{
    return SystemGetDataPoolItemFull(vulkanShaderLibraryPool, shaderLibrary);
}

VulkanPipelineStateData* GetVulkanPipelineStateData(ElemPipelineState pipelineState)
{
    return SystemGetDataPoolItem(vulkanPipelineStatePool, pipelineState);
}

VulkanPipelineStateDataFull* GetVulkanPipelineStateDataFull(ElemPipelineState pipelineState)
{
    return SystemGetDataPoolItemFull(vulkanPipelineStatePool, pipelineState);
}

ElemShaderLibrary VulkanCreateShaderLibrary(ElemGraphicsDevice graphicsDevice, ElemDataSpan shaderLibraryData)
{
    InitVulkanShaderMemory();

    // TODO: Handle true libraries if possible?

    SystemAssert(graphicsDevice != ELEM_HANDLE_NULL);
    
    auto graphicsDeviceData = GetVulkanGraphicsDeviceData(graphicsDevice);
    SystemAssert(graphicsDeviceData);

    auto dataSpan = Span<uint8_t>(shaderLibraryData.Items, shaderLibraryData.Length); 
    auto shaderCount = *(uint32_t*)dataSpan.Pointer;
    dataSpan = dataSpan.Slice(sizeof(uint32_t));

    // HACK: This is bad to allocate this here but this should be temporary
    auto graphicsShaderData = SystemPushArray<VkShaderModule>(VulkanGraphicsMemoryArena, shaderCount);

    for (uint32_t i = 0; i < shaderCount; i++)
    {
        auto size = *(uint32_t*)dataSpan.Pointer;
        dataSpan = dataSpan.Slice(sizeof(uint32_t));

        // TODO: Debug
        auto dest = SystemPushArray<uint8_t>(VulkanGraphicsMemoryArena, size);
        SystemCopyBuffer(dest, ReadOnlySpan<uint8_t>(dataSpan.Pointer, size));

        VkShaderModuleCreateInfo createInfo = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        createInfo.codeSize = (size_t)size;
        createInfo.pCode = reinterpret_cast<const uint32_t*>(dataSpan.Pointer); // TODO: Get rid of the reinterpret_cast

        VkShaderModule shaderModule = nullptr;
        AssertIfFailed(vkCreateShaderModule(graphicsDeviceData->Device, &createInfo, 0, &shaderModule));

        graphicsShaderData[i] = shaderModule;
        dataSpan = dataSpan.Slice(size);
    }

    auto handle = SystemAddDataPoolItem(vulkanShaderLibraryPool, {
        .GraphicsShaders = graphicsShaderData
    }); 

    SystemAddDataPoolItemFull(vulkanShaderLibraryPool, handle, {
        .GraphicsDevice = graphicsDevice
    });
    
    return handle;
}

void VulkanFreeShaderLibrary(ElemShaderLibrary shaderLibrary)
{
    SystemAssert(shaderLibrary != ELEM_HANDLE_NULL);

    auto shaderLibraryData = GetVulkanShaderLibraryData(shaderLibrary);
    SystemAssert(shaderLibraryData);

    auto shaderLibraryDataFull = GetVulkanShaderLibraryDataFull(shaderLibrary);
    SystemAssert(shaderLibraryDataFull);

    auto graphicsDeviceData = GetVulkanGraphicsDeviceData(shaderLibraryDataFull->GraphicsDevice);
    SystemAssert(graphicsDeviceData);

    for (uint32_t i = 0; i < shaderLibraryData->GraphicsShaders.Length; i++)
    {
        vkDestroyShaderModule(graphicsDeviceData->Device, shaderLibraryData->GraphicsShaders[i], nullptr);
    }
}

ElemPipelineState VulkanCompileGraphicsPipelineState(ElemGraphicsDevice graphicsDevice, const ElemGraphicsPipelineStateParameters* parameters)
{
    // TODO: To Refactor
    // TODO: Support libraries

    InitVulkanShaderMemory();

    SystemAssert(graphicsDevice != ELEM_HANDLE_NULL);

    auto graphicsDeviceData = GetVulkanGraphicsDeviceData(graphicsDevice);
    SystemAssert(graphicsDeviceData);

    SystemAssert(parameters);
    SystemAssert(parameters->ShaderLibrary != ELEM_HANDLE_NULL);

    auto shaderLibraryData= GetVulkanShaderLibraryData(parameters->ShaderLibrary);
    SystemAssert(shaderLibraryData);

    VkGraphicsPipelineCreateInfo createInfo = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
    uint32_t stagesCount = 2;

    VkPipelineShaderStageCreateInfo stages[3] = {};
    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_MESH_BIT_EXT;
    stages[0].module = shaderLibraryData->GraphicsShaders[0];
    stages[0].pName = parameters->MeshShaderFunction;

    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = shaderLibraryData->GraphicsShaders[1];
    stages[1].pName = parameters->PixelShaderFunction;
    
    // TODO: Amplification shader 
    
    createInfo.stageCount = stagesCount;
    createInfo.pStages = stages;

    VkPipelineViewportStateCreateInfo viewportState = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;
    createInfo.pViewportState = &viewportState;

    VkPipelineRasterizationStateCreateInfo rasterizationState = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
    rasterizationState.lineWidth = 1.0f;
    rasterizationState.cullMode = VK_CULL_MODE_NONE;//VK_CULL_MODE_BACK_BIT;
    rasterizationState.frontFace = VK_FRONT_FACE_CLOCKWISE;
    createInfo.pRasterizationState = &rasterizationState;

    VkPipelineMultisampleStateCreateInfo multisampleState = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
    multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    createInfo.pMultisampleState = &multisampleState;

    VkPipelineDepthStencilStateCreateInfo depthStencilState = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
    createInfo.pDepthStencilState = &depthStencilState;

    // TODO: To Refactor!    
    VkPipelineColorBlendAttachmentState renderTargetBlendState = {
				false,
				VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO, VK_BLEND_OP_ADD,
				VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO, VK_BLEND_OP_ADD,
				VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
			};

    VkPipelineColorBlendStateCreateInfo colorBlendState = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
    colorBlendState.attachmentCount = 1;
    colorBlendState.pAttachments = &renderTargetBlendState;
    createInfo.pColorBlendState = &colorBlendState;

    VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

    VkPipelineDynamicStateCreateInfo dynamicState = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
    dynamicState.dynamicStateCount = sizeof(dynamicStates) / sizeof(dynamicStates[0]);
    dynamicState.pDynamicStates = dynamicStates;
    createInfo.pDynamicState = &dynamicState;

    VkFormat formats[] = {VK_FORMAT_B8G8R8A8_SRGB};

    VkPipelineRenderingCreateInfo renderingCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
    renderingCreateInfo.colorAttachmentCount = 1; // TODO: Change that
    renderingCreateInfo.pColorAttachmentFormats = formats;
    createInfo.pNext = &renderingCreateInfo;
    createInfo.layout = graphicsDeviceData->PipelineLayout;

    VkPipeline pipelineState;
    AssertIfFailed(vkCreateGraphicsPipelines(graphicsDeviceData->Device, nullptr, 1, &createInfo, 0, &pipelineState));

    auto handle = SystemAddDataPoolItem(vulkanPipelineStatePool, {
        .PipelineState = pipelineState,
        .GraphicsDevice = graphicsDevice
    }); 

    SystemAddDataPoolItemFull(vulkanPipelineStatePool, handle, {
    });

    return handle;
}

void VulkanFreePipelineState(ElemPipelineState pipelineState)
{
    SystemAssert(pipelineState != ELEM_HANDLE_NULL);

    auto pipelineStateData = GetVulkanPipelineStateData(pipelineState);
    SystemAssert(pipelineStateData);
    
    auto graphicsDeviceData = GetVulkanGraphicsDeviceData(pipelineStateData->GraphicsDevice);
    SystemAssert(graphicsDeviceData);

    vkDeviceWaitIdle(graphicsDeviceData->Device);
    vkDestroyPipeline(graphicsDeviceData->Device, pipelineStateData->PipelineState, nullptr);
}

void VulkanBindPipelineState(ElemCommandList commandList, ElemPipelineState pipelineState)
{
    SystemAssert(commandList != ELEM_HANDLE_NULL);
    SystemAssert(pipelineState != ELEM_HANDLE_NULL);

    auto commandListData = GetVulkanCommandListData(commandList);
    SystemAssert(commandListData);

    auto pipelineStateData = GetVulkanPipelineStateData(pipelineState);
    SystemAssert(pipelineStateData);

    vkCmdBindPipeline(commandListData->DeviceObject, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineStateData->PipelineState);
}

void VulkanPushPipelineStateConstants(ElemCommandList commandList, uint32_t offsetInBytes, ElemDataSpan data)
{
    SystemAssert(commandList != ELEM_HANDLE_NULL);

    auto commandListData = GetVulkanCommandListData(commandList);
    SystemAssert(commandListData);

    auto graphicsDeviceData = GetVulkanGraphicsDeviceData(commandListData->GraphicsDevice);
    SystemAssert(graphicsDeviceData);

    vkCmdPushConstants(commandListData->DeviceObject, graphicsDeviceData->PipelineLayout, VK_SHADER_STAGE_ALL, 0, data.Length, data.Items);
}
