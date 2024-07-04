#include "Elemental.h"
#include "GraphicsTests.h"
#include "utest.h"

// TODO: Validate dispatch thread group count
// TODO: Cannot push constant before binding pso

UTEST(Shader, CreateComputePipelineState) 
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

UTEST(Shader, CreateComputePipelineStateFunctionNotExist) 
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
    auto readbackBuffer = TestCreateReadbackBuffer(graphicsDevice, 64 * sizeof(uint32_t));

    // Act
    TestDispatchComputeForReadbackBuffer(graphicsDevice, commandQueue, "ShaderTests.shader", "TestCompute", 1, 1, 1, &readbackBuffer.Descriptor);

    // Assert
    auto bufferData = ElemGetGraphicsResourceDataSpan(readbackBuffer.Buffer);

    TestFreeReadbackBuffer(readbackBuffer);
    ElemFreeCommandQueue(commandQueue);
    ElemFreeGraphicsDevice(graphicsDevice);

    ASSERT_LOG_NOERROR();
    auto uintData = (uint32_t*)bufferData.Items;

    for (uint32_t i = 0; i < bufferData.Length / 4; i++)
    {
        ASSERT_EQ_MSG(uintData[i], i < 16 ? i : 0u, "Compute shader data is invalid.");
    }
}
