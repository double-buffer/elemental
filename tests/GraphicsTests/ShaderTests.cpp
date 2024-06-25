#include "Elemental.h"
#include "GraphicsTests.h"
#include "utest.h"

UTEST(Shader, CreateComputePipelineState) 
{
    // Arrange
    auto graphicsDevice = TestGetSharedGraphicsDevice();
    auto shaderLibrary = TestOpenShader("ShaderTests.shader");
    ASSERT_NE(ELEM_HANDLE_NULL, shaderLibrary);

    // Act
    ElemComputePipelineStateParameters parameters =
    {
        .ShaderLibrary = shaderLibrary,
        .ComputeShaderFunction = "TestCompute"
    };

    auto pipelineState = ElemCompileComputePipelineState(graphicsDevice, &parameters); 

    // Assert
    ASSERT_NE(ELEM_HANDLE_NULL, pipelineState);
    ASSERT_LOG_NOERROR();

    ElemFreePipelineState(pipelineState);
}

UTEST(Shader, CreateComputePipelineStateFunctionNotExist) 
{
    // Arrange
    auto graphicsDevice = TestGetSharedGraphicsDevice();
    auto shaderLibrary = TestOpenShader("ShaderTests.shader");
    ASSERT_NE(shaderLibrary, ELEM_HANDLE_NULL);

    // Act
    ElemComputePipelineStateParameters parameters =
    {
        .ShaderLibrary = shaderLibrary,
        .ComputeShaderFunction = "TestComputeNotExist"
    };

    auto pipelineState = ElemCompileComputePipelineState(graphicsDevice, &parameters); 

    // Assert
    ASSERT_EQ(ELEM_HANDLE_NULL, pipelineState);
    ASSERT_LOG_MESSAGE("Cannot find compute shader function");
}

UTEST(Shader, DispatchComputeWithoutBindPipelineState) 
{
    // Arrange
    auto commandQueue = TestGetSharedCommandQueue();
    auto commandList = ElemGetCommandList(commandQueue, nullptr);

    // Act
    ElemDispatchCompute(commandList, 1, 0, 0);

    // Assert
    ASSERT_TRUE_MSG(testHasLogErrors, "Validation logs should have errors.");
    ASSERT_LOG_MESSAGE("A compute pipelinestate must be bound to the commandlist before calling a compute command.");

    ElemCommitCommandList(commandList);
}

UTEST(Shader, DispatchCompute) 
{
    // Arrange
    auto readbackBuffer = TestCreateReadbackBuffer(64 * sizeof(uint32_t));

    // Act
    TestDispatchComputeForReadbackBuffer("ShaderTests.shader", "TestCompute", 1, 1, 1, &readbackBuffer.Descriptor);

    // Assert
    ASSERT_LOG_NOERROR();

    auto bufferData = ElemGetGraphicsResourceDataSpan(readbackBuffer.Buffer);
    auto uintData = (uint32_t*)bufferData.Items;

    for (uint32_t i = 0; i < bufferData.Length / 4; i++)
    {
        ASSERT_EQ_MSG(uintData[i], i < 16 ? i : 0u, "Compute shader data is invalid.");
    }

    TestFreeReadbackBuffer(readbackBuffer);
}

// TODO: Validate dispatch thread group count
// TODO: Cannot push constant before binding pso
