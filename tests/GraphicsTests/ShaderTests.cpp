#include "Elemental.h"
#include "GraphicsTests.h"
#include "utest.h"

// TODO: Validate dispatch thread group count
// TODO: Cannot push constant before binding pso
// TODO: PSO Multi render targets (Test Blend States too)
// TODO: Check depth stencil format if comparaison function set

UTEST(Shader, CompileComputePipelineState) 
{
    // Arrange
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);
    auto shaderLibrary = TestOpenShader(graphicsDevice, "ShaderTests.shader");
    ASSERT_NE(ELEM_HANDLE_NULL, shaderLibrary);

    // Act
    ElemComputePipelineStateParameters parameters =
    {
        .ShaderLibrary = shaderLibrary,
        .ComputeShaderFunction = "TestCompute"
    };

    auto pipelineState = ElemCompileComputePipelineState(graphicsDevice, &parameters); 

    // Assert
    ElemFreeShaderLibrary(shaderLibrary);
    ElemFreePipelineState(pipelineState);
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_LOG_NOERROR();
    ASSERT_NE(ELEM_HANDLE_NULL, pipelineState);
}

UTEST(Shader, CompileComputePipelineStateFunctionNotExist) 
{
    // Arrange
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);
    auto shaderLibrary = TestOpenShader(graphicsDevice, "ShaderTests.shader");
    ASSERT_NE(shaderLibrary, ELEM_HANDLE_NULL);

    // Act
    ElemComputePipelineStateParameters parameters =
    {
        .ShaderLibrary = shaderLibrary,
        .ComputeShaderFunction = "TestComputeNotExist"
    };

    auto pipelineState = ElemCompileComputePipelineState(graphicsDevice, &parameters); 

    // Assert
    ElemFreeShaderLibrary(shaderLibrary);
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_LOG_MESSAGE("Cannot find shader function");
    ASSERT_EQ(ELEM_HANDLE_NULL, pipelineState);
}

UTEST(Shader, DispatchComputeWithoutBindPipelineState) 
{
    // Arrange
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);
    auto commandQueue = ElemCreateCommandQueue(graphicsDevice, ElemCommandQueueType_Graphics, nullptr);
    auto commandList = ElemGetCommandList(commandQueue, nullptr);

    // Act
    ElemDispatchCompute(commandList, 1, 0, 0);

    // Assert
    ElemCommitCommandList(commandList);

    ElemFreeCommandQueue(commandQueue);
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_TRUE_MSG(testHasLogErrors, "Validation logs should have errors.");
    ASSERT_LOG_MESSAGE("A compute pipelinestate must be bound to the commandlist before calling a compute command.");
}

UTEST(Shader, DispatchCompute) 
{
    // Arrange
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);
    auto commandQueue = ElemCreateCommandQueue(graphicsDevice, ElemCommandQueueType_Graphics, nullptr);
    auto readbackBuffer = TestCreateGpuBuffer(graphicsDevice, 64 * sizeof(uint32_t), ElemGraphicsHeapType_Readback);

    // Act
    TestDispatchComputeForReadbackBuffer(graphicsDevice, commandQueue, "ShaderTests.shader", "TestCompute", 1, 1, 1, &readbackBuffer.WriteDescriptor);

    // Assert
    auto bufferData = ElemDownloadGraphicsBufferData(readbackBuffer.Buffer, nullptr);

    TestFreeGpuBuffer(readbackBuffer);
    ElemFreeCommandQueue(commandQueue);
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_LOG_NOERROR();
    auto uintData = (uint32_t*)bufferData.Items;

    for (uint32_t i = 0; i < bufferData.Length / 4; i++)
    {
        ASSERT_EQ_MSG(uintData[i], i < 16 ? i : 0u, "Compute shader data is invalid.");
    }
}

struct Shader_CompileGraphicsPipelineStateFillAndCullMode
{
    ElemGraphicsFillMode FillMode;
    ElemGraphicsCullMode CullMode;
    bool TwoColors;
    float Color[3];
    float Color2[3];
};

UTEST_F_SETUP(Shader_CompileGraphicsPipelineStateFillAndCullMode) 
{
}

UTEST_F_TEARDOWN(Shader_CompileGraphicsPipelineStateFillAndCullMode) 
{
    // Arrange
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);
    auto commandQueue = ElemCreateCommandQueue(graphicsDevice, ElemCommandQueueType_Graphics, nullptr);
    auto commandList = ElemGetCommandList(commandQueue, nullptr);

    auto renderTarget = TestCreateGpuTexture(graphicsDevice, 16, 16, ElemGraphicsFormat_R32G32B32A32_FLOAT, ElemGraphicsResourceUsage_RenderTarget);
    
    ElemGraphicsPipelineStateRenderTarget psoRenderTarget { .Format = renderTarget.Format };
    ElemGraphicsPipelineStateParameters psoParameters =
    {
        .RenderTargets = { .Items = &psoRenderTarget, .Length = 1 },
        .FillMode = utest_fixture->FillMode,
        .CullMode = utest_fixture->CullMode
    };

    auto meshShaderPipeline = TestOpenMeshShader(graphicsDevice, "ShaderTests.shader", "MeshShader", "PixelShader", &psoParameters);

    // Act
    ElemRenderPassRenderTarget renderPassRenderTarget = 
    {
        .RenderTarget = renderTarget.Texture,
        .ClearColor = { .Red = 0.0f, .Green = 0.0f, .Blue = 1.0f, .Alpha = 1.0f },
        .LoadAction = ElemRenderPassLoadAction_Clear
    };

    ElemBeginRenderPassParameters parameters =
    {
        .RenderTargets =
        { 
            .Items = &renderPassRenderTarget,
            .Length = 1
        }
    };

    ElemBeginRenderPass(commandList, &parameters);
    ElemBindPipelineState(commandList, meshShaderPipeline);

    float shaderParameters[] = { 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f };
    ElemPushPipelineStateConstants(commandList, 0, { .Items = (uint8_t*)shaderParameters, .Length = sizeof(float) * 8 });
    ElemDispatchMesh(commandList, 1, 1, 1);

    ElemEndRenderPass(commandList);
    
    // Assert
    ElemCommitCommandList(commandList);
    auto fence = ElemExecuteCommandList(commandQueue, commandList, nullptr);
    ElemWaitForFenceOnCpu(fence);

    auto readbackBuffer = TestCreateGpuBuffer(graphicsDevice, 16 * 16 * 4 * sizeof(float), ElemGraphicsHeapType_Readback);
    uint32_t resourceIdList[] = { (uint32_t)renderTarget.ReadDescriptor, (uint32_t)readbackBuffer.WriteDescriptor };
    TestDispatchComputeForReadbackBuffer(graphicsDevice, commandQueue, "Assert.shader", "CopyTexture", 1, 1, 1, &resourceIdList);
    auto bufferData = ElemDownloadGraphicsBufferData(readbackBuffer.Buffer, nullptr);

    TestFreeGpuBuffer(readbackBuffer);
    TestFreeGpuTexture(renderTarget);
    ElemFreePipelineState(meshShaderPipeline);
    ElemFreeCommandQueue(commandQueue);
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_LOG_NOERROR();

    auto floatData = (float*)bufferData.Items;
    bool colorMatch[2] = {};

    for (uint32_t i = 0; i < bufferData.Length / sizeof(float); i += 4)
    {
        if (!utest_fixture->TwoColors)
        {
            ASSERT_EQ_MSG(floatData[i], utest_fixture->Color[0], "Red channel data is invalid.");
            ASSERT_EQ_MSG(floatData[i + 1], utest_fixture->Color[1], "Green channel data is invalid.");
            ASSERT_EQ_MSG(floatData[i + 2], utest_fixture->Color[2], "Blue channel data is invalid.");
            ASSERT_EQ_MSG(floatData[i + 3], 1.0f, "Alpha channel data is invalid.");
        }
        else
        {
            if (floatData[i] == utest_fixture->Color[0] &&
                floatData[i + 1] == utest_fixture->Color[1] &&
                floatData[i + 2] == utest_fixture->Color[2])
            {
                colorMatch[0] = true;
            }
            else if(floatData[i] == utest_fixture->Color2[0] &&
                    floatData[i + 1] == utest_fixture->Color2[1] &&
                    floatData[i + 2] == utest_fixture->Color2[2])
            {
                colorMatch[1] = true;
            }
        }
    }

    if (utest_fixture->TwoColors)
    {
        ASSERT_TRUE_MSG(colorMatch[0], "First color must be present.");
        ASSERT_TRUE_MSG(colorMatch[1], "Second color must be present.");
    }
}

UTEST_F(Shader_CompileGraphicsPipelineStateFillAndCullMode, FillModeSolid) 
{
    utest_fixture->FillMode = ElemGraphicsFillMode_Solid;
    utest_fixture->Color[0] = 1.0f;
    utest_fixture->Color[1] = 1.0f;
    utest_fixture->Color[2] = 0.0f;
}

UTEST_F(Shader_CompileGraphicsPipelineStateFillAndCullMode, FillModeWireframe) 
{
    utest_fixture->FillMode = ElemGraphicsFillMode_Wireframe;

    utest_fixture->TwoColors = true;
    utest_fixture->Color[0] = 0.0f;
    utest_fixture->Color[1] = 0.0f;
    utest_fixture->Color[2] = 1.0f;

    utest_fixture->Color2[0] = 1.0f;
    utest_fixture->Color2[1] = 1.0f;
    utest_fixture->Color2[2] = 0.0f;
}

UTEST_F(Shader_CompileGraphicsPipelineStateFillAndCullMode, CullModeBackface) 
{
    utest_fixture->CullMode = ElemGraphicsCullMode_BackFace;
    utest_fixture->Color[0] = 1.0f;
    utest_fixture->Color[1] = 1.0f;
    utest_fixture->Color[2] = 0.0f;
}

UTEST_F(Shader_CompileGraphicsPipelineStateFillAndCullMode, CullModeFrontFace) 
{
    utest_fixture->CullMode = ElemGraphicsCullMode_FrontFace;
    utest_fixture->Color[0] = 0.0f;
    utest_fixture->Color[1] = 0.0f;
    utest_fixture->Color[2] = 1.0f;
}

UTEST_F(Shader_CompileGraphicsPipelineStateFillAndCullMode, CullModeNone) 
{
    utest_fixture->CullMode = ElemGraphicsCullMode_None;
    utest_fixture->Color[0] = 1.0f;
    utest_fixture->Color[1] = 1.0f;
    utest_fixture->Color[2] = 0.0f;
}

struct Shader_CompileGraphicsPipelineStateDepthCompare
{
    float ClearDepthValue;
    ElemGraphicsCompareFunction CompareFunction;
    float Color[3];
};

UTEST_F_SETUP(Shader_CompileGraphicsPipelineStateDepthCompare) 
{
}

UTEST_F_TEARDOWN(Shader_CompileGraphicsPipelineStateDepthCompare) 
{
    // Arrange
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);
    auto commandQueue = ElemCreateCommandQueue(graphicsDevice, ElemCommandQueueType_Graphics, nullptr);
    auto commandList = ElemGetCommandList(commandQueue, nullptr);

    auto renderTarget = TestCreateGpuTexture(graphicsDevice, 16, 16, ElemGraphicsFormat_R32G32B32A32_FLOAT, ElemGraphicsResourceUsage_RenderTarget);
    auto depthBuffer = TestCreateGpuTexture(graphicsDevice, 16, 16, ElemGraphicsFormat_D32_FLOAT, ElemGraphicsResourceUsage_DepthStencil);
    
    ElemGraphicsPipelineStateRenderTarget psoRenderTarget { .Format = renderTarget.Format };
    ElemGraphicsPipelineStateParameters psoParameters =
    {
        .RenderTargets = { .Items = &psoRenderTarget, .Length = 1 },
        .DepthStencil = 
        {
            .Format = depthBuffer.Format,
            .DepthCompareFunction = utest_fixture->CompareFunction
        }
    };

    auto meshShaderPipeline = TestOpenMeshShader(graphicsDevice, "ShaderTests.shader", "MeshShader", "PixelShader", &psoParameters);

    // Act
    ElemRenderPassRenderTarget renderPassRenderTarget = 
    {
        .RenderTarget = renderTarget.Texture,
        .ClearColor = { .Red = 0.0f, .Green = 0.0f, .Blue = 0.0f, .Alpha = 1.0f },
        .LoadAction = ElemRenderPassLoadAction_Clear
    };

    ElemBeginRenderPassParameters parameters =
    {
        .RenderTargets =
        { 
            .Items = &renderPassRenderTarget,
            .Length = 1
        },
        .DepthStencil = 
        {
            .DepthStencil = depthBuffer.Texture,
            .DepthClearValue = utest_fixture->ClearDepthValue
        }
    };

    ElemBeginRenderPass(commandList, &parameters);
    ElemBindPipelineState(commandList, meshShaderPipeline);

    float shaderParameters[] = { 0.0f, 0.25f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f };
    ElemPushPipelineStateConstants(commandList, 0, { .Items = (uint8_t*)shaderParameters, .Length = sizeof(float) * 8 });
    ElemDispatchMesh(commandList, 1, 1, 1);

    float shaderParameters2[] = { 0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f };
    ElemPushPipelineStateConstants(commandList, 0, { .Items = (uint8_t*)shaderParameters2, .Length = sizeof(float) * 8 });
    ElemDispatchMesh(commandList, 1, 1, 1);

    float shaderParameters3[] = { 0.0f, 0.75f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f };
    ElemPushPipelineStateConstants(commandList, 0, { .Items = (uint8_t*)shaderParameters3, .Length = sizeof(float) * 8 });
    ElemDispatchMesh(commandList, 1, 1, 1);

    ElemEndRenderPass(commandList);
    
    // Assert
    ElemCommitCommandList(commandList);
    auto fence = ElemExecuteCommandList(commandQueue, commandList, nullptr);
    ElemWaitForFenceOnCpu(fence);

    auto readbackBuffer = TestCreateGpuBuffer(graphicsDevice, 16 * 16 * 4 * sizeof(float), ElemGraphicsHeapType_Readback);
    uint32_t resourceIdList[] = { (uint32_t)renderTarget.ReadDescriptor, (uint32_t)readbackBuffer.WriteDescriptor };
    TestDispatchComputeForReadbackBuffer(graphicsDevice, commandQueue, "Assert.shader", "CopyTexture", 1, 1, 1, &resourceIdList);
    auto bufferData = ElemDownloadGraphicsBufferData(readbackBuffer.Buffer, nullptr);

    TestFreeGpuBuffer(readbackBuffer);
    TestFreeGpuTexture(renderTarget);
    TestFreeGpuTexture(depthBuffer);
    ElemFreePipelineState(meshShaderPipeline);
    ElemFreeCommandQueue(commandQueue);
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_LOG_NOERROR();
    ASSERT_COLOR_BUFFER(bufferData, utest_fixture->Color[0], utest_fixture->Color[1], utest_fixture->Color[2], 1.0f);
}

UTEST_F(Shader_CompileGraphicsPipelineStateDepthCompare, Never) 
{
    utest_fixture->ClearDepthValue = 0.5f;
    utest_fixture->CompareFunction = ElemGraphicsCompareFunction_Never;
    utest_fixture->Color[0] = 0.0f;
    utest_fixture->Color[1] = 0.0f;
    utest_fixture->Color[2] = 0.0f;
}

UTEST_F(Shader_CompileGraphicsPipelineStateDepthCompare, Less) 
{
    utest_fixture->ClearDepthValue = 1.0f;
    utest_fixture->CompareFunction = ElemGraphicsCompareFunction_Less;
    utest_fixture->Color[0] = 1.0f;
    utest_fixture->Color[1] = 0.0f;
    utest_fixture->Color[2] = 0.0f;
}

UTEST_F(Shader_CompileGraphicsPipelineStateDepthCompare, LessEqual) 
{
    utest_fixture->ClearDepthValue = 0.25f;
    utest_fixture->CompareFunction = ElemGraphicsCompareFunction_LessEqual;
    utest_fixture->Color[0] = 1.0f;
    utest_fixture->Color[1] = 0.0f;
    utest_fixture->Color[2] = 0.0f;
}

UTEST_F(Shader_CompileGraphicsPipelineStateDepthCompare, Greater) 
{
    utest_fixture->ClearDepthValue = 0.0f;
    utest_fixture->CompareFunction = ElemGraphicsCompareFunction_Greater;
    utest_fixture->Color[0] = 0.0f;
    utest_fixture->Color[1] = 0.0f;
    utest_fixture->Color[2] = 1.0f;
}

UTEST_F(Shader_CompileGraphicsPipelineStateDepthCompare, GreaterEqual) 
{
    utest_fixture->ClearDepthValue = 0.75f;
    utest_fixture->CompareFunction = ElemGraphicsCompareFunction_GreaterEqual;
    utest_fixture->Color[0] = 0.0f;
    utest_fixture->Color[1] = 0.0f;
    utest_fixture->Color[2] = 1.0f;
}

UTEST_F(Shader_CompileGraphicsPipelineStateDepthCompare, Equal) 
{
    utest_fixture->ClearDepthValue = 0.5f;
    utest_fixture->CompareFunction = ElemGraphicsCompareFunction_Equal;
    utest_fixture->Color[0] = 0.0f;
    utest_fixture->Color[1] = 1.0f;
    utest_fixture->Color[2] = 0.0f;
}

UTEST_F(Shader_CompileGraphicsPipelineStateDepthCompare, Always) 
{
    utest_fixture->ClearDepthValue = 1.0f;
    utest_fixture->CompareFunction = ElemGraphicsCompareFunction_Always;
    utest_fixture->Color[0] = 0.0f;
    utest_fixture->Color[1] = 0.0f;
    utest_fixture->Color[2] = 1.0f;
}

struct Shader_CompileGraphicsPipelineStateBlendState
{
    ElemGraphicsBlendOperation BlendOperation;
    ElemGraphicsBlendFactor SourceBlendFactor;
    ElemGraphicsBlendFactor DestinationBlendFactor;
    ElemGraphicsBlendOperation BlendOperationAlpha;
    ElemGraphicsBlendFactor SourceBlendFactorAlpha;
    ElemGraphicsBlendFactor DestinationBlendFactorAlpha;
    float DestinationColor[4];
    float SourceColor[4];
    float ResultColor[4];
};

UTEST_F_SETUP(Shader_CompileGraphicsPipelineStateBlendState) 
{
}

UTEST_F_TEARDOWN(Shader_CompileGraphicsPipelineStateBlendState) 
{
    // Arrange
    auto graphicsDevice = ElemCreateGraphicsDevice(nullptr);
    auto commandQueue = ElemCreateCommandQueue(graphicsDevice, ElemCommandQueueType_Graphics, nullptr);
    auto commandList = ElemGetCommandList(commandQueue, nullptr);

    auto renderTarget = TestCreateGpuTexture(graphicsDevice, 16, 16, ElemGraphicsFormat_R32G32B32A32_FLOAT, ElemGraphicsResourceUsage_RenderTarget);
    
    ElemGraphicsPipelineStateRenderTarget psoRenderTarget = 
    { 
        .Format = renderTarget.Format,
        .BlendOperation = utest_fixture->BlendOperation,
        .SourceBlendFactor = utest_fixture->SourceBlendFactor,
        .DestinationBlendFactor = utest_fixture->DestinationBlendFactor,
        .BlendOperationAlpha = utest_fixture->BlendOperationAlpha,
        .SourceBlendFactorAlpha = utest_fixture->SourceBlendFactorAlpha,
        .DestinationBlendFactorAlpha = utest_fixture->DestinationBlendFactorAlpha
    };

    ElemGraphicsPipelineStateParameters psoParameters =
    {
        .RenderTargets = { .Items = &psoRenderTarget, .Length = 1 },
    };

    auto meshShaderPipeline = TestOpenMeshShader(graphicsDevice, "ShaderTests.shader", "MeshShader", "PixelShader", &psoParameters);

    // Act
    ElemRenderPassRenderTarget renderPassRenderTarget = 
    {
        .RenderTarget = renderTarget.Texture,
        .ClearColor = 
        { 
            .Red = utest_fixture->DestinationColor[0], 
            .Green = utest_fixture->DestinationColor[1], 
            .Blue = utest_fixture->DestinationColor[2], 
            .Alpha = utest_fixture->DestinationColor[3]
        },
        .LoadAction = ElemRenderPassLoadAction_Clear
    };

    ElemBeginRenderPassParameters parameters =
    {
        .RenderTargets =
        { 
            .Items = &renderPassRenderTarget,
            .Length = 1
        }
    };

    ElemBeginRenderPass(commandList, &parameters);
    ElemBindPipelineState(commandList, meshShaderPipeline);

    float shaderParameters[] = { 0.0f, 0.0f, 0.0f, 0.0f, utest_fixture->SourceColor[0], utest_fixture->SourceColor[1], utest_fixture->SourceColor[2], utest_fixture->SourceColor[3] };
    ElemPushPipelineStateConstants(commandList, 0, { .Items = (uint8_t*)shaderParameters, .Length = sizeof(float) * 8 });
    ElemDispatchMesh(commandList, 1, 1, 1);

    ElemEndRenderPass(commandList);
    
    // Assert
    ElemCommitCommandList(commandList);
    auto fence = ElemExecuteCommandList(commandQueue, commandList, nullptr);
    ElemWaitForFenceOnCpu(fence);

    auto readbackBuffer = TestCreateGpuBuffer(graphicsDevice, 16 * 16 * 4 * sizeof(float), ElemGraphicsHeapType_Readback);
    uint32_t resourceIdList[] = { (uint32_t)renderTarget.ReadDescriptor, (uint32_t)readbackBuffer.WriteDescriptor };
    TestDispatchComputeForReadbackBuffer(graphicsDevice, commandQueue, "Assert.shader", "CopyTexture", 1, 1, 1, &resourceIdList);
    auto bufferData = ElemDownloadGraphicsBufferData(readbackBuffer.Buffer, nullptr);

    TestFreeGpuBuffer(readbackBuffer);
    TestFreeGpuTexture(renderTarget);
    ElemFreePipelineState(meshShaderPipeline);
    ElemFreeCommandQueue(commandQueue);
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_LOG_NOERROR();

    auto floatData = (float*)bufferData.Items;

    for (uint32_t i = 0; i < bufferData.Length / sizeof(float); i += 4)
    {
        ASSERT_EQ_MSG(floatData[i], utest_fixture->ResultColor[0], "Red channel data is invalid.");
        ASSERT_EQ_MSG(floatData[i + 1], utest_fixture->ResultColor[1], "Green channel data is invalid.");
        ASSERT_EQ_MSG(floatData[i + 2], utest_fixture->ResultColor[2], "Blue channel data is invalid.");
        ASSERT_EQ_MSG(floatData[i + 3], utest_fixture->ResultColor[3], "Alpha channel data is invalid.");
    }
}

UTEST_F(Shader_CompileGraphicsPipelineStateBlendState, Defaults) 
{
    utest_fixture->BlendOperation = ElemGraphicsBlendOperation_Add;
    utest_fixture->SourceBlendFactor = ElemGraphicsBlendFactor_Zero;
    utest_fixture->DestinationBlendFactor = ElemGraphicsBlendFactor_Zero;

    utest_fixture->BlendOperationAlpha = ElemGraphicsBlendOperation_Add;
    utest_fixture->SourceBlendFactorAlpha = ElemGraphicsBlendFactor_Zero;
    utest_fixture->DestinationBlendFactorAlpha = ElemGraphicsBlendFactor_Zero;

    utest_fixture->DestinationColor[0] = 0.0f;
    utest_fixture->DestinationColor[1] = 1.0f;
    utest_fixture->DestinationColor[2] = 0.0f;
    utest_fixture->DestinationColor[3] = 1.0f;

    utest_fixture->SourceColor[0] = 1.0f;
    utest_fixture->SourceColor[1] = 0.5f;
    utest_fixture->SourceColor[2] = 1.0f;
    utest_fixture->SourceColor[3] = 1.0f;

    utest_fixture->ResultColor[0] = 1.0f;
    utest_fixture->ResultColor[1] = 0.5f;
    utest_fixture->ResultColor[2] = 1.0f;
    utest_fixture->ResultColor[3] = 1.0f;
}

UTEST_F(Shader_CompileGraphicsPipelineStateBlendState, Add_Zero_One) 
{
    utest_fixture->BlendOperation = ElemGraphicsBlendOperation_Add;
    utest_fixture->SourceBlendFactor = ElemGraphicsBlendFactor_Zero;
    utest_fixture->DestinationBlendFactor = ElemGraphicsBlendFactor_One;

    utest_fixture->BlendOperationAlpha = ElemGraphicsBlendOperation_Add;
    utest_fixture->SourceBlendFactorAlpha = ElemGraphicsBlendFactor_Zero;
    utest_fixture->DestinationBlendFactorAlpha = ElemGraphicsBlendFactor_Zero;

    utest_fixture->DestinationColor[0] = 0.0f;
    utest_fixture->DestinationColor[1] = 1.0f;
    utest_fixture->DestinationColor[2] = 0.0f;
    utest_fixture->DestinationColor[3] = 1.0f;

    utest_fixture->SourceColor[0] = 1.0f;
    utest_fixture->SourceColor[1] = 0.5f;
    utest_fixture->SourceColor[2] = 1.0f;
    utest_fixture->SourceColor[3] = 1.0f;

    utest_fixture->ResultColor[0] = 0.0f;
    utest_fixture->ResultColor[1] = 1.0f;
    utest_fixture->ResultColor[2] = 0.0f;
    utest_fixture->ResultColor[3] = 0.0f;
}

UTEST_F(Shader_CompileGraphicsPipelineStateBlendState, Add_One_Zero) 
{
    utest_fixture->BlendOperation = ElemGraphicsBlendOperation_Add;
    utest_fixture->SourceBlendFactor = ElemGraphicsBlendFactor_One;
    utest_fixture->DestinationBlendFactor = ElemGraphicsBlendFactor_Zero;

    utest_fixture->BlendOperationAlpha = ElemGraphicsBlendOperation_Add;
    utest_fixture->SourceBlendFactorAlpha = ElemGraphicsBlendFactor_Zero;
    utest_fixture->DestinationBlendFactorAlpha = ElemGraphicsBlendFactor_Zero;

    utest_fixture->DestinationColor[0] = 0.0f;
    utest_fixture->DestinationColor[1] = 1.0f;
    utest_fixture->DestinationColor[2] = 0.0f;
    utest_fixture->DestinationColor[3] = 1.0f;

    utest_fixture->SourceColor[0] = 1.0f;
    utest_fixture->SourceColor[1] = 0.5f;
    utest_fixture->SourceColor[2] = 1.0f;
    utest_fixture->SourceColor[3] = 1.0f;

    utest_fixture->ResultColor[0] = 1.0f;
    utest_fixture->ResultColor[1] = 0.5f;
    utest_fixture->ResultColor[2] = 1.0f;
    utest_fixture->ResultColor[3] = 0.0f;
}

UTEST_F(Shader_CompileGraphicsPipelineStateBlendState, Add_One_One) 
{
    utest_fixture->BlendOperation = ElemGraphicsBlendOperation_Add;
    utest_fixture->SourceBlendFactor = ElemGraphicsBlendFactor_One;
    utest_fixture->DestinationBlendFactor = ElemGraphicsBlendFactor_One;

    utest_fixture->BlendOperationAlpha = ElemGraphicsBlendOperation_Add;
    utest_fixture->SourceBlendFactorAlpha = ElemGraphicsBlendFactor_Zero;
    utest_fixture->DestinationBlendFactorAlpha = ElemGraphicsBlendFactor_Zero;

    utest_fixture->DestinationColor[0] = 0.0f;
    utest_fixture->DestinationColor[1] = 1.0f;
    utest_fixture->DestinationColor[2] = 0.0f;
    utest_fixture->DestinationColor[3] = 1.0f;

    utest_fixture->SourceColor[0] = 1.0f;
    utest_fixture->SourceColor[1] = 0.5f;
    utest_fixture->SourceColor[2] = 1.0f;
    utest_fixture->SourceColor[3] = 1.0f;

    utest_fixture->ResultColor[0] = 1.0f;
    utest_fixture->ResultColor[1] = 1.5f;
    utest_fixture->ResultColor[2] = 1.0f;
    utest_fixture->ResultColor[3] = 0.0f;
}

UTEST_F(Shader_CompileGraphicsPipelineStateBlendState, AddAlpha_Zero_One) 
{
    utest_fixture->BlendOperation = ElemGraphicsBlendOperation_Add;
    utest_fixture->SourceBlendFactor = ElemGraphicsBlendFactor_Zero;
    utest_fixture->DestinationBlendFactor = ElemGraphicsBlendFactor_Zero;

    utest_fixture->BlendOperationAlpha = ElemGraphicsBlendOperation_Add;
    utest_fixture->SourceBlendFactorAlpha = ElemGraphicsBlendFactor_Zero;
    utest_fixture->DestinationBlendFactorAlpha = ElemGraphicsBlendFactor_One;

    utest_fixture->DestinationColor[0] = 0.0f;
    utest_fixture->DestinationColor[1] = 1.0f;
    utest_fixture->DestinationColor[2] = 0.0f;
    utest_fixture->DestinationColor[3] = 0.5f;

    utest_fixture->SourceColor[0] = 1.0f;
    utest_fixture->SourceColor[1] = 0.5f;
    utest_fixture->SourceColor[2] = 1.0f;
    utest_fixture->SourceColor[3] = 1.0f;

    utest_fixture->ResultColor[0] = 0.0f;
    utest_fixture->ResultColor[1] = 0.0f;
    utest_fixture->ResultColor[2] = 0.0f;
    utest_fixture->ResultColor[3] = 0.5f;
}

UTEST_F(Shader_CompileGraphicsPipelineStateBlendState, AddAlpha_One_Zero) 
{
    utest_fixture->BlendOperation = ElemGraphicsBlendOperation_Add;
    utest_fixture->SourceBlendFactor = ElemGraphicsBlendFactor_Zero;
    utest_fixture->DestinationBlendFactor = ElemGraphicsBlendFactor_Zero;

    utest_fixture->BlendOperationAlpha = ElemGraphicsBlendOperation_Add;
    utest_fixture->SourceBlendFactorAlpha = ElemGraphicsBlendFactor_One;
    utest_fixture->DestinationBlendFactorAlpha = ElemGraphicsBlendFactor_Zero;

    utest_fixture->DestinationColor[0] = 0.0f;
    utest_fixture->DestinationColor[1] = 1.0f;
    utest_fixture->DestinationColor[2] = 0.0f;
    utest_fixture->DestinationColor[3] = 0.5f;

    utest_fixture->SourceColor[0] = 1.0f;
    utest_fixture->SourceColor[1] = 0.5f;
    utest_fixture->SourceColor[2] = 1.0f;
    utest_fixture->SourceColor[3] = 1.0f;

    utest_fixture->ResultColor[0] = 0.0f;
    utest_fixture->ResultColor[1] = 0.0f;
    utest_fixture->ResultColor[2] = 0.0f;
    utest_fixture->ResultColor[3] = 1.0f;
}

UTEST_F(Shader_CompileGraphicsPipelineStateBlendState, AddAlpha_One_One) 
{
    utest_fixture->BlendOperation = ElemGraphicsBlendOperation_Add;
    utest_fixture->SourceBlendFactor = ElemGraphicsBlendFactor_Zero;
    utest_fixture->DestinationBlendFactor = ElemGraphicsBlendFactor_Zero;

    utest_fixture->BlendOperationAlpha = ElemGraphicsBlendOperation_Add;
    utest_fixture->SourceBlendFactorAlpha = ElemGraphicsBlendFactor_One;
    utest_fixture->DestinationBlendFactorAlpha = ElemGraphicsBlendFactor_One;

    utest_fixture->DestinationColor[0] = 0.0f;
    utest_fixture->DestinationColor[1] = 1.0f;
    utest_fixture->DestinationColor[2] = 0.0f;
    utest_fixture->DestinationColor[3] = 0.5f;

    utest_fixture->SourceColor[0] = 1.0f;
    utest_fixture->SourceColor[1] = 0.5f;
    utest_fixture->SourceColor[2] = 1.0f;
    utest_fixture->SourceColor[3] = 1.0f;

    utest_fixture->ResultColor[0] = 0.0f;
    utest_fixture->ResultColor[1] = 0.0f;
    utest_fixture->ResultColor[2] = 0.0f;
    utest_fixture->ResultColor[3] = 1.5f;
}

UTEST_F(Shader_CompileGraphicsPipelineStateBlendState, Add_SourceColor_One) 
{
    utest_fixture->BlendOperation = ElemGraphicsBlendOperation_Add;
    utest_fixture->SourceBlendFactor = ElemGraphicsBlendFactor_SourceColor;
    utest_fixture->DestinationBlendFactor = ElemGraphicsBlendFactor_One;

    utest_fixture->BlendOperationAlpha = ElemGraphicsBlendOperation_Add;
    utest_fixture->SourceBlendFactorAlpha = ElemGraphicsBlendFactor_Zero;
    utest_fixture->DestinationBlendFactorAlpha = ElemGraphicsBlendFactor_Zero;

    utest_fixture->DestinationColor[0] = 0.0f;
    utest_fixture->DestinationColor[1] = 1.0f;
    utest_fixture->DestinationColor[2] = 0.0f;
    utest_fixture->DestinationColor[3] = 1.0f;

    utest_fixture->SourceColor[0] = 1.0f;
    utest_fixture->SourceColor[1] = 0.5f;
    utest_fixture->SourceColor[2] = 1.0f;
    utest_fixture->SourceColor[3] = 1.0f;

    utest_fixture->ResultColor[0] = 1.0f;
    utest_fixture->ResultColor[1] = 1.25f;
    utest_fixture->ResultColor[2] = 1.0f;
    utest_fixture->ResultColor[3] = 0.0f;
}

UTEST_F(Shader_CompileGraphicsPipelineStateBlendState, Add_InverseSourceColor_One) 
{
    utest_fixture->BlendOperation = ElemGraphicsBlendOperation_Add;
    utest_fixture->SourceBlendFactor = ElemGraphicsBlendFactor_InverseSourceColor;
    utest_fixture->DestinationBlendFactor = ElemGraphicsBlendFactor_One;

    utest_fixture->BlendOperationAlpha = ElemGraphicsBlendOperation_Add;
    utest_fixture->SourceBlendFactorAlpha = ElemGraphicsBlendFactor_Zero;
    utest_fixture->DestinationBlendFactorAlpha = ElemGraphicsBlendFactor_Zero;

    utest_fixture->DestinationColor[0] = 0.0f;
    utest_fixture->DestinationColor[1] = 1.0f;
    utest_fixture->DestinationColor[2] = 0.0f;
    utest_fixture->DestinationColor[3] = 1.0f;

    utest_fixture->SourceColor[0] = 1.0f;
    utest_fixture->SourceColor[1] = 0.25f;
    utest_fixture->SourceColor[2] = 1.0f;
    utest_fixture->SourceColor[3] = 1.0f;

    utest_fixture->ResultColor[0] = 0.0f;
    utest_fixture->ResultColor[1] = 1.1875f;
    utest_fixture->ResultColor[2] = 0.0f;
    utest_fixture->ResultColor[3] = 0.0f;
}

UTEST_F(Shader_CompileGraphicsPipelineStateBlendState, Add_SourceAlpha_One) 
{
    utest_fixture->BlendOperation = ElemGraphicsBlendOperation_Add;
    utest_fixture->SourceBlendFactor = ElemGraphicsBlendFactor_SourceAlpha;
    utest_fixture->DestinationBlendFactor = ElemGraphicsBlendFactor_One;

    utest_fixture->BlendOperationAlpha = ElemGraphicsBlendOperation_Add;
    utest_fixture->SourceBlendFactorAlpha = ElemGraphicsBlendFactor_Zero;
    utest_fixture->DestinationBlendFactorAlpha = ElemGraphicsBlendFactor_Zero;

    utest_fixture->DestinationColor[0] = 0.0f;
    utest_fixture->DestinationColor[1] = 1.0f;
    utest_fixture->DestinationColor[2] = 0.0f;
    utest_fixture->DestinationColor[3] = 1.0f;

    utest_fixture->SourceColor[0] = 1.0f;
    utest_fixture->SourceColor[1] = 0.25f;
    utest_fixture->SourceColor[2] = 0.8;
    utest_fixture->SourceColor[3] = 0.5f;

    utest_fixture->ResultColor[0] = 0.5f;
    utest_fixture->ResultColor[1] = 1.125f;
    utest_fixture->ResultColor[2] = 0.4f;
    utest_fixture->ResultColor[3] = 0.0f;
}

UTEST_F(Shader_CompileGraphicsPipelineStateBlendState, Add_InverseSourceAlpha_One) 
{
    utest_fixture->BlendOperation = ElemGraphicsBlendOperation_Add;
    utest_fixture->SourceBlendFactor = ElemGraphicsBlendFactor_InverseSourceAlpha;
    utest_fixture->DestinationBlendFactor = ElemGraphicsBlendFactor_One;

    utest_fixture->BlendOperationAlpha = ElemGraphicsBlendOperation_Add;
    utest_fixture->SourceBlendFactorAlpha = ElemGraphicsBlendFactor_Zero;
    utest_fixture->DestinationBlendFactorAlpha = ElemGraphicsBlendFactor_Zero;

    utest_fixture->DestinationColor[0] = 0.0f;
    utest_fixture->DestinationColor[1] = 1.0f;
    utest_fixture->DestinationColor[2] = 0.0f;
    utest_fixture->DestinationColor[3] = 1.0f;

    utest_fixture->SourceColor[0] = 1.0f;
    utest_fixture->SourceColor[1] = 0.5f;
    utest_fixture->SourceColor[2] = 0.8;
    utest_fixture->SourceColor[3] = 0.75f;

    utest_fixture->ResultColor[0] = 0.25f;
    utest_fixture->ResultColor[1] = 1.125f;
    utest_fixture->ResultColor[2] = 0.2f;
    utest_fixture->ResultColor[3] = 0.0f;
}

UTEST_F(Shader_CompileGraphicsPipelineStateBlendState, Add_DestinationColor_One) 
{
    utest_fixture->BlendOperation = ElemGraphicsBlendOperation_Add;
    utest_fixture->SourceBlendFactor = ElemGraphicsBlendFactor_DestinationColor;
    utest_fixture->DestinationBlendFactor = ElemGraphicsBlendFactor_One;

    utest_fixture->BlendOperationAlpha = ElemGraphicsBlendOperation_Add;
    utest_fixture->SourceBlendFactorAlpha = ElemGraphicsBlendFactor_Zero;
    utest_fixture->DestinationBlendFactorAlpha = ElemGraphicsBlendFactor_Zero;

    utest_fixture->DestinationColor[0] = 0.5f;
    utest_fixture->DestinationColor[1] = 1.0f;
    utest_fixture->DestinationColor[2] = 0.0f;
    utest_fixture->DestinationColor[3] = 1.0f;

    utest_fixture->SourceColor[0] = 1.0f;
    utest_fixture->SourceColor[1] = 0.5f;
    utest_fixture->SourceColor[2] = 1.0f;
    utest_fixture->SourceColor[3] = 1.0f;

    utest_fixture->ResultColor[0] = 1.0f;
    utest_fixture->ResultColor[1] = 1.5f;
    utest_fixture->ResultColor[2] = 0.0f;
    utest_fixture->ResultColor[3] = 0.0f;
}

UTEST_F(Shader_CompileGraphicsPipelineStateBlendState, Add_InverseDestinationColor_One) 
{
    utest_fixture->BlendOperation = ElemGraphicsBlendOperation_Add;
    utest_fixture->SourceBlendFactor = ElemGraphicsBlendFactor_InverseDestinationColor;
    utest_fixture->DestinationBlendFactor = ElemGraphicsBlendFactor_One;

    utest_fixture->BlendOperationAlpha = ElemGraphicsBlendOperation_Add;
    utest_fixture->SourceBlendFactorAlpha = ElemGraphicsBlendFactor_Zero;
    utest_fixture->DestinationBlendFactorAlpha = ElemGraphicsBlendFactor_Zero;

    utest_fixture->DestinationColor[0] = 0.5f;
    utest_fixture->DestinationColor[1] = 1.0f;
    utest_fixture->DestinationColor[2] = 0.0f;
    utest_fixture->DestinationColor[3] = 1.0f;

    utest_fixture->SourceColor[0] = 1.0f;
    utest_fixture->SourceColor[1] = 0.25f;
    utest_fixture->SourceColor[2] = 1.0f;
    utest_fixture->SourceColor[3] = 1.0f;

    utest_fixture->ResultColor[0] = 1.0f;
    utest_fixture->ResultColor[1] = 1.0f;
    utest_fixture->ResultColor[2] = 1.0f;
    utest_fixture->ResultColor[3] = 0.0f;
}

UTEST_F(Shader_CompileGraphicsPipelineStateBlendState, Add_DestinationAlpha_One) 
{
    utest_fixture->BlendOperation = ElemGraphicsBlendOperation_Add;
    utest_fixture->SourceBlendFactor = ElemGraphicsBlendFactor_DestinationAlpha;
    utest_fixture->DestinationBlendFactor = ElemGraphicsBlendFactor_One;

    utest_fixture->BlendOperationAlpha = ElemGraphicsBlendOperation_Add;
    utest_fixture->SourceBlendFactorAlpha = ElemGraphicsBlendFactor_Zero;
    utest_fixture->DestinationBlendFactorAlpha = ElemGraphicsBlendFactor_Zero;

    utest_fixture->DestinationColor[0] = 0.5f;
    utest_fixture->DestinationColor[1] = 1.0f;
    utest_fixture->DestinationColor[2] = 0.0f;
    utest_fixture->DestinationColor[3] = 1.0f;

    utest_fixture->SourceColor[0] = 1.0f;
    utest_fixture->SourceColor[1] = 0.25f;
    utest_fixture->SourceColor[2] = 0.8;
    utest_fixture->SourceColor[3] = 0.5f;

    utest_fixture->ResultColor[0] = 1.5f;
    utest_fixture->ResultColor[1] = 1.25;
    utest_fixture->ResultColor[2] = 0.8f;
    utest_fixture->ResultColor[3] = 0.0f;
}

UTEST_F(Shader_CompileGraphicsPipelineStateBlendState, Add_InverseDestinationAlpha_One) 
{
    utest_fixture->BlendOperation = ElemGraphicsBlendOperation_Add;
    utest_fixture->SourceBlendFactor = ElemGraphicsBlendFactor_InverseDestinationAlpha;
    utest_fixture->DestinationBlendFactor = ElemGraphicsBlendFactor_One;

    utest_fixture->BlendOperationAlpha = ElemGraphicsBlendOperation_Add;
    utest_fixture->SourceBlendFactorAlpha = ElemGraphicsBlendFactor_Zero;
    utest_fixture->DestinationBlendFactorAlpha = ElemGraphicsBlendFactor_Zero;

    utest_fixture->DestinationColor[0] = 0.5f;
    utest_fixture->DestinationColor[1] = 1.0f;
    utest_fixture->DestinationColor[2] = 0.0f;
    utest_fixture->DestinationColor[3] = 1.0f;

    utest_fixture->SourceColor[0] = 1.0f;
    utest_fixture->SourceColor[1] = 0.5f;
    utest_fixture->SourceColor[2] = 0.8;
    utest_fixture->SourceColor[3] = 0.75f;

    utest_fixture->ResultColor[0] = 0.5f;
    utest_fixture->ResultColor[1] = 1.0f;
    utest_fixture->ResultColor[2] = 0.0f;
    utest_fixture->ResultColor[3] = 0.0f;
}

UTEST_F(Shader_CompileGraphicsPipelineStateBlendState, Add_SourceAlphaSaturated_One) 
{
    utest_fixture->BlendOperation = ElemGraphicsBlendOperation_Add;
    utest_fixture->SourceBlendFactor = ElemGraphicsBlendFactor_SourceAlphaSaturated;
    utest_fixture->DestinationBlendFactor = ElemGraphicsBlendFactor_One;

    utest_fixture->BlendOperationAlpha = ElemGraphicsBlendOperation_Add;
    utest_fixture->SourceBlendFactorAlpha = ElemGraphicsBlendFactor_Zero;
    utest_fixture->DestinationBlendFactorAlpha = ElemGraphicsBlendFactor_Zero;

    utest_fixture->DestinationColor[0] = 0.0f;
    utest_fixture->DestinationColor[1] = 1.0f;
    utest_fixture->DestinationColor[2] = 0.0f;
    utest_fixture->DestinationColor[3] = 0.75f;

    utest_fixture->SourceColor[0] = 1.0f;
    utest_fixture->SourceColor[1] = 0.25f;
    utest_fixture->SourceColor[2] = 0.8;
    utest_fixture->SourceColor[3] = 0.5f;

    utest_fixture->ResultColor[0] = 0.25f;
    utest_fixture->ResultColor[1] = 1.0625f;
    utest_fixture->ResultColor[2] = 0.2f;
    utest_fixture->ResultColor[3] = 0.0f;
}

UTEST_F(Shader_CompileGraphicsPipelineStateBlendState, Subtract_One_One) 
{
    utest_fixture->BlendOperation = ElemGraphicsBlendOperation_Subtract;
    utest_fixture->SourceBlendFactor = ElemGraphicsBlendFactor_One;
    utest_fixture->DestinationBlendFactor = ElemGraphicsBlendFactor_One;

    utest_fixture->BlendOperationAlpha = ElemGraphicsBlendOperation_Add;
    utest_fixture->SourceBlendFactorAlpha = ElemGraphicsBlendFactor_Zero;
    utest_fixture->DestinationBlendFactorAlpha = ElemGraphicsBlendFactor_Zero;

    utest_fixture->DestinationColor[0] = 0.25f;
    utest_fixture->DestinationColor[1] = 0.5f;
    utest_fixture->DestinationColor[2] = 0.75f;
    utest_fixture->DestinationColor[3] = 0.5f;

    utest_fixture->SourceColor[0] = 1.0f;
    utest_fixture->SourceColor[1] = 1.0f;
    utest_fixture->SourceColor[2] = 1.0f;
    utest_fixture->SourceColor[3] = 1.0f;

    utest_fixture->ResultColor[0] = 0.75f;
    utest_fixture->ResultColor[1] = 0.5f;
    utest_fixture->ResultColor[2] = 0.25f;
    utest_fixture->ResultColor[3] = 0.0f;
}

UTEST_F(Shader_CompileGraphicsPipelineStateBlendState, ReverseSubtract_One_One) 
{
    utest_fixture->BlendOperation = ElemGraphicsBlendOperation_ReverseSubtract;
    utest_fixture->SourceBlendFactor = ElemGraphicsBlendFactor_One;
    utest_fixture->DestinationBlendFactor = ElemGraphicsBlendFactor_One;

    utest_fixture->BlendOperationAlpha = ElemGraphicsBlendOperation_Add;
    utest_fixture->SourceBlendFactorAlpha = ElemGraphicsBlendFactor_Zero;
    utest_fixture->DestinationBlendFactorAlpha = ElemGraphicsBlendFactor_Zero;

    utest_fixture->DestinationColor[0] = 1.0f;
    utest_fixture->DestinationColor[1] = 1.0f;
    utest_fixture->DestinationColor[2] = 1.0f;
    utest_fixture->DestinationColor[3] = 1.0f;

    utest_fixture->SourceColor[0] = 0.25f;
    utest_fixture->SourceColor[1] = 0.5f;
    utest_fixture->SourceColor[2] = 0.75f;
    utest_fixture->SourceColor[3] = 0.5f;

    utest_fixture->ResultColor[0] = 0.75f;
    utest_fixture->ResultColor[1] = 0.5f;
    utest_fixture->ResultColor[2] = 0.25f;
    utest_fixture->ResultColor[3] = 0.0f;
}

UTEST_F(Shader_CompileGraphicsPipelineStateBlendState, Min_One_One) 
{
    utest_fixture->BlendOperation = ElemGraphicsBlendOperation_Min;
    utest_fixture->SourceBlendFactor = ElemGraphicsBlendFactor_One;
    utest_fixture->DestinationBlendFactor = ElemGraphicsBlendFactor_One;

    utest_fixture->BlendOperationAlpha = ElemGraphicsBlendOperation_Add;
    utest_fixture->SourceBlendFactorAlpha = ElemGraphicsBlendFactor_Zero;
    utest_fixture->DestinationBlendFactorAlpha = ElemGraphicsBlendFactor_Zero;

    utest_fixture->DestinationColor[0] = 1.0f;
    utest_fixture->DestinationColor[1] = 1.0f;
    utest_fixture->DestinationColor[2] = 1.0f;
    utest_fixture->DestinationColor[3] = 1.0f;

    utest_fixture->SourceColor[0] = 0.25f;
    utest_fixture->SourceColor[1] = 0.5f;
    utest_fixture->SourceColor[2] = 0.75f;
    utest_fixture->SourceColor[3] = 0.5f;

    utest_fixture->ResultColor[0] = 0.25f;
    utest_fixture->ResultColor[1] = 0.5f;
    utest_fixture->ResultColor[2] = 0.75f;
    utest_fixture->ResultColor[3] = 0.0f;
}

UTEST_F(Shader_CompileGraphicsPipelineStateBlendState, Max_One_One) 
{
    utest_fixture->BlendOperation = ElemGraphicsBlendOperation_Max;
    utest_fixture->SourceBlendFactor = ElemGraphicsBlendFactor_One;
    utest_fixture->DestinationBlendFactor = ElemGraphicsBlendFactor_One;

    utest_fixture->BlendOperationAlpha = ElemGraphicsBlendOperation_Add;
    utest_fixture->SourceBlendFactorAlpha = ElemGraphicsBlendFactor_Zero;
    utest_fixture->DestinationBlendFactorAlpha = ElemGraphicsBlendFactor_Zero;

    utest_fixture->DestinationColor[0] = 1.0f;
    utest_fixture->DestinationColor[1] = 1.0f;
    utest_fixture->DestinationColor[2] = 1.0f;
    utest_fixture->DestinationColor[3] = 1.0f;

    utest_fixture->SourceColor[0] = 0.25f;
    utest_fixture->SourceColor[1] = 0.5f;
    utest_fixture->SourceColor[2] = 0.75f;
    utest_fixture->SourceColor[3] = 0.5f;

    utest_fixture->ResultColor[0] = 1.0f;
    utest_fixture->ResultColor[1] = 1.0f;
    utest_fixture->ResultColor[2] = 1.0f;
    utest_fixture->ResultColor[3] = 0.0f;
}

UTEST_F(Shader_CompileGraphicsPipelineStateBlendState, AlphaSubtract_One_One) 
{
    utest_fixture->BlendOperation = ElemGraphicsBlendOperation_Add;
    utest_fixture->SourceBlendFactor = ElemGraphicsBlendFactor_Zero;
    utest_fixture->DestinationBlendFactor = ElemGraphicsBlendFactor_Zero;

    utest_fixture->BlendOperationAlpha = ElemGraphicsBlendOperation_Subtract;
    utest_fixture->SourceBlendFactorAlpha = ElemGraphicsBlendFactor_One;
    utest_fixture->DestinationBlendFactorAlpha = ElemGraphicsBlendFactor_One;

    utest_fixture->DestinationColor[0] = 0.25f;
    utest_fixture->DestinationColor[1] = 0.5f;
    utest_fixture->DestinationColor[2] = 0.75f;
    utest_fixture->DestinationColor[3] = 0.5f;

    utest_fixture->SourceColor[0] = 1.0f;
    utest_fixture->SourceColor[1] = 1.0f;
    utest_fixture->SourceColor[2] = 1.0f;
    utest_fixture->SourceColor[3] = 1.0f;

    utest_fixture->ResultColor[0] = 0.0f;
    utest_fixture->ResultColor[1] = 0.0f;
    utest_fixture->ResultColor[2] = 0.0f;
    utest_fixture->ResultColor[3] = 0.5f;
}

UTEST_F(Shader_CompileGraphicsPipelineStateBlendState, AlphaReverseSubtract_One_One) 
{
    utest_fixture->BlendOperation = ElemGraphicsBlendOperation_Add;
    utest_fixture->SourceBlendFactor = ElemGraphicsBlendFactor_Zero;
    utest_fixture->DestinationBlendFactor = ElemGraphicsBlendFactor_Zero;

    utest_fixture->BlendOperationAlpha = ElemGraphicsBlendOperation_ReverseSubtract;
    utest_fixture->SourceBlendFactorAlpha = ElemGraphicsBlendFactor_One;
    utest_fixture->DestinationBlendFactorAlpha = ElemGraphicsBlendFactor_One;

    utest_fixture->DestinationColor[0] = 1.0f;
    utest_fixture->DestinationColor[1] = 1.0f;
    utest_fixture->DestinationColor[2] = 1.0f;
    utest_fixture->DestinationColor[3] = 1.0f;

    utest_fixture->SourceColor[0] = 0.25f;
    utest_fixture->SourceColor[1] = 0.5f;
    utest_fixture->SourceColor[2] = 0.75f;
    utest_fixture->SourceColor[3] = 0.5f;

    utest_fixture->ResultColor[0] = 0.0f;
    utest_fixture->ResultColor[1] = 0.0f;
    utest_fixture->ResultColor[2] = 0.0f;
    utest_fixture->ResultColor[3] = 0.5f;
}

UTEST_F(Shader_CompileGraphicsPipelineStateBlendState, AlphaMin_One_One) 
{
    utest_fixture->BlendOperation = ElemGraphicsBlendOperation_Add;
    utest_fixture->SourceBlendFactor = ElemGraphicsBlendFactor_Zero;
    utest_fixture->DestinationBlendFactor = ElemGraphicsBlendFactor_Zero;

    utest_fixture->BlendOperationAlpha = ElemGraphicsBlendOperation_Min;
    utest_fixture->SourceBlendFactorAlpha = ElemGraphicsBlendFactor_One;
    utest_fixture->DestinationBlendFactorAlpha = ElemGraphicsBlendFactor_One;

    utest_fixture->DestinationColor[0] = 1.0f;
    utest_fixture->DestinationColor[1] = 1.0f;
    utest_fixture->DestinationColor[2] = 1.0f;
    utest_fixture->DestinationColor[3] = 1.0f;

    utest_fixture->SourceColor[0] = 0.25f;
    utest_fixture->SourceColor[1] = 0.5f;
    utest_fixture->SourceColor[2] = 0.75f;
    utest_fixture->SourceColor[3] = 0.5f;

    utest_fixture->ResultColor[0] = 0.0f;
    utest_fixture->ResultColor[1] = 0.0f;
    utest_fixture->ResultColor[2] = 0.0f;
    utest_fixture->ResultColor[3] = 0.5f;
}

UTEST_F(Shader_CompileGraphicsPipelineStateBlendState, AlphaMax_One_One) 
{
    utest_fixture->BlendOperation = ElemGraphicsBlendOperation_Add;
    utest_fixture->SourceBlendFactor = ElemGraphicsBlendFactor_Zero;
    utest_fixture->DestinationBlendFactor = ElemGraphicsBlendFactor_Zero;

    utest_fixture->BlendOperationAlpha = ElemGraphicsBlendOperation_Max;
    utest_fixture->SourceBlendFactorAlpha = ElemGraphicsBlendFactor_One;
    utest_fixture->DestinationBlendFactorAlpha = ElemGraphicsBlendFactor_One;

    utest_fixture->DestinationColor[0] = 1.0f;
    utest_fixture->DestinationColor[1] = 1.0f;
    utest_fixture->DestinationColor[2] = 1.0f;
    utest_fixture->DestinationColor[3] = 1.0f;

    utest_fixture->SourceColor[0] = 0.25f;
    utest_fixture->SourceColor[1] = 0.5f;
    utest_fixture->SourceColor[2] = 0.75f;
    utest_fixture->SourceColor[3] = 0.5f;

    utest_fixture->ResultColor[0] = 0.0f;
    utest_fixture->ResultColor[1] = 0.0f;
    utest_fixture->ResultColor[2] = 0.0f;
    utest_fixture->ResultColor[3] = 1.0f;
}
