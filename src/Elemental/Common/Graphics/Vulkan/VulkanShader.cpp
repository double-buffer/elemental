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

bool CheckVulkanPipelineStateType(const VulkanCommandListData* commandListData, VulkanPipelineStateType type)
{
    if (commandListData->PipelineStateType != type)
    {
        if (type == VulkanPipelineStateType_Compute)
        {
            SystemLogErrorMessage(ElemLogMessageCategory_Graphics, "A compute pipelinestate must be bound to the commandlist before calling a compute command.");
        }
        else
        {
            SystemLogErrorMessage(ElemLogMessageCategory_Graphics, "A graphics pipelinestate must be bound to the commandlist before calling a rendering command.");
        }

        return false;
    }

    return true;
}

VkShaderStageFlagBits ConvertShaderTypeToVulkan(ShaderType shaderType)
{
    switch (shaderType) 
    {
        case ShaderType_Amplification:
            return VK_SHADER_STAGE_TASK_BIT_EXT;

        case ShaderType_Mesh:
            return VK_SHADER_STAGE_MESH_BIT_EXT;

        case ShaderType_Pixel:
            return VK_SHADER_STAGE_FRAGMENT_BIT;

        case ShaderType_Compute:
            return VK_SHADER_STAGE_COMPUTE_BIT;

        case ShaderType_Library:
        case ShaderType_Unknown:
            return VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
    }
}

VkPipelineShaderStageCreateInfo GetVulkanShaderFunctionStageCreateInfo(MemoryArena memoryArena, VulkanShaderLibraryData* shaderLibraryData, ShaderType shaderType, const char* function)
{
    SystemAssert(function);
    VkPipelineShaderStageCreateInfo result = {};
    result.stage = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;

    for (uint32_t i = 0; i < shaderLibraryData->GraphicsShaders.Length; i++)
    {
        auto shader = shaderLibraryData->GraphicsShaders[i];

        if (shader.ShaderType == shaderType && SystemFindSubString(shader.Name, function) != -1)
        {
            auto moduleCreateInfo = SystemPushStruct<VkShaderModuleCreateInfo>(memoryArena);
            moduleCreateInfo->sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            moduleCreateInfo->pCode = (uint32_t*)shader.ShaderCode.Pointer;
            moduleCreateInfo->codeSize = shader.ShaderCode.Length;

            VkPipelineShaderStageCreateInfo stageCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
            stageCreateInfo.stage = ConvertShaderTypeToVulkan(shaderType);
            stageCreateInfo.pName = function;
            stageCreateInfo.pNext = moduleCreateInfo;

            result = stageCreateInfo;
            break;
        }
    }
        
    if (result.stage == VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM)
    {
        SystemLogErrorMessage(ElemLogMessageCategory_Graphics, "Cannot find shader function '%s'", function);
    }

    return result;
}

ElemShaderLibrary VulkanCreateShaderLibrary(ElemGraphicsDevice graphicsDevice, ElemDataSpan shaderLibraryData)
{
    InitVulkanShaderMemory();

    // TODO: Handle true libraries if possible?

    SystemAssert(graphicsDevice != ELEM_HANDLE_NULL);
    
    auto graphicsDeviceData = GetVulkanGraphicsDeviceData(graphicsDevice);
    SystemAssert(graphicsDeviceData);

    auto dataSpan = Span<uint8_t>(shaderLibraryData.Items, shaderLibraryData.Length); 
    auto graphicsShaderData = ReadShaders(VulkanGraphicsMemoryArena, dataSpan);

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
    // TODO: Free data
}

ElemPipelineState VulkanCompileGraphicsPipelineState(ElemGraphicsDevice graphicsDevice, const ElemGraphicsPipelineStateParameters* parameters)
{
    // TODO: Support libraries

    InitVulkanShaderMemory();
    auto stackMemoryArena = SystemGetStackMemoryArena();

    SystemAssert(graphicsDevice != ELEM_HANDLE_NULL);

    auto graphicsDeviceData = GetVulkanGraphicsDeviceData(graphicsDevice);
    SystemAssert(graphicsDeviceData);

    SystemAssert(parameters);
    SystemAssert(parameters->ShaderLibrary != ELEM_HANDLE_NULL);

    auto shaderLibraryData= GetVulkanShaderLibraryData(parameters->ShaderLibrary);
    SystemAssert(shaderLibraryData);

    VkGraphicsPipelineCreateInfo createInfo = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };

    VkPipelineShaderStageCreateInfo stages[3] = {};
    uint32_t stageCount = 0;

    // TODO: Amplification shader 

    if (parameters->MeshShaderFunction)
    {
        auto shaderStage = GetVulkanShaderFunctionStageCreateInfo(stackMemoryArena, shaderLibraryData, ShaderType_Mesh, parameters->MeshShaderFunction);

        if (shaderStage.stage == VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM)
        {
            return ELEM_HANDLE_NULL;
        }

        stages[stageCount++] = shaderStage;
    }
    
    if (parameters->PixelShaderFunction)
    {
        auto shaderStage = GetVulkanShaderFunctionStageCreateInfo(stackMemoryArena, shaderLibraryData, ShaderType_Pixel, parameters->PixelShaderFunction);

        if (shaderStage.stage == VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM)
        {
            return ELEM_HANDLE_NULL;
        }

        stages[stageCount++] = shaderStage;
    }
    
    createInfo.stageCount = stageCount;
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
        .PipelineStateType = VulkanPipelineStateType_Graphics,
        .GraphicsDevice = graphicsDevice
    }); 

    SystemAddDataPoolItemFull(vulkanPipelineStatePool, handle, {
    });

    return handle;
}

ElemPipelineState VulkanCompileComputePipelineState(ElemGraphicsDevice graphicsDevice, const ElemComputePipelineStateParameters* parameters)
{
    InitVulkanShaderMemory();
    auto stackMemoryArena = SystemGetStackMemoryArena();

    SystemAssert(graphicsDevice != ELEM_HANDLE_NULL);

    auto graphicsDeviceData = GetVulkanGraphicsDeviceData(graphicsDevice);
    SystemAssert(graphicsDeviceData);

    SystemAssert(parameters);
    SystemAssert(parameters->ShaderLibrary != ELEM_HANDLE_NULL);

    auto shaderLibraryData= GetVulkanShaderLibraryData(parameters->ShaderLibrary);
    SystemAssert(shaderLibraryData);

    VkComputePipelineCreateInfo createInfo = { VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };

    if (parameters->ComputeShaderFunction)
    {
        auto shaderStage = GetVulkanShaderFunctionStageCreateInfo(stackMemoryArena, shaderLibraryData, ShaderType_Compute, parameters->ComputeShaderFunction);

        if (shaderStage.stage == VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM)
        {
            return ELEM_HANDLE_NULL;
        }

        createInfo.stage = shaderStage;
    }

    createInfo.layout = graphicsDeviceData->PipelineLayout;

	VkPipeline pipelineState;
	AssertIfFailed(vkCreateComputePipelines(graphicsDeviceData->Device, nullptr, 1, &createInfo, 0, &pipelineState));

    auto handle = SystemAddDataPoolItem(vulkanPipelineStatePool, {
        .PipelineState = pipelineState,
        .PipelineStateType = VulkanPipelineStateType_Compute,
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

    commandListData->PipelineStateType = pipelineStateData->PipelineStateType;

    vkCmdBindPipeline(commandListData->DeviceObject, 
                      (pipelineStateData->PipelineStateType == VulkanPipelineStateType_Graphics) ? VK_PIPELINE_BIND_POINT_GRAPHICS : VK_PIPELINE_BIND_POINT_COMPUTE, 
                      pipelineStateData->PipelineState);
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

void VulkanDispatchCompute(ElemCommandList commandList, uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ)
{
    SystemAssert(commandList != ELEM_HANDLE_NULL);

    auto commandListData = GetVulkanCommandListData(commandList);
    SystemAssert(commandListData);

    if (!CheckVulkanPipelineStateType(commandListData, VulkanPipelineStateType_Compute))
    {
        return;
    }

    vkCmdDispatch(commandListData->DeviceObject, threadGroupCountX, threadGroupCountY, threadGroupCountZ);
}
