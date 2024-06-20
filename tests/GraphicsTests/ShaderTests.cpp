#include "Elemental.h"
#include "GraphicsTests.h"
#include "utest.h"

ElemShaderLibrary OpenShader(const char* shader)
{
    auto graphicsDevice = GetSharedGraphicsDevice();

    // TODO: Vulkan shaders on windows
    auto shaderData = ReadFile(shader);
    auto shaderLibrary = ElemCreateShaderLibrary(graphicsDevice, shaderData);
    return shaderLibrary;
}

UTEST(Shader, CreateComputePipelineState) 
{
    // Arrange
    InitLog();
    auto graphicsDevice = GetSharedGraphicsDevice();
    auto shaderLibrary = OpenShader("ShaderTests.shader");
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
    ASSERT_FALSE_MSG(testHasLogErrors, testErrorLogs);

    ElemFreePipelineState(pipelineState);
}

UTEST(Shader, CreateComputePipelineStateFunctionNotExist) 
{
    // Arrange
    InitLog();
    auto graphicsDevice = GetSharedGraphicsDevice();
    auto shaderLibrary = OpenShader("ShaderTests.shader");
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
    ASSERT_TRUE_MSG(testHasLogErrors, "Validation logs should have errors.");
    ASSERT_LOG("Cannot find compute shader function");
}

UTEST(Shader, DispatchComputeWithoutBindPipelineState) 
{
    // Arrange
    InitLog();
    auto graphicsDevice = GetSharedGraphicsDevice();
    auto commandQueue = ElemCreateCommandQueue(graphicsDevice, ElemCommandQueueType_Graphics, nullptr);
    auto commandList = ElemGetCommandList(commandQueue, nullptr);

    // Act
    ElemDispatchCompute(commandList, 1, 0, 0);

    // Assert
    ASSERT_TRUE_MSG(testHasLogErrors, "Validation logs should have errors.");
    ASSERT_LOG("A compute pipelinestate must be bound to the commandlist before calling a compute command.");

    ElemFreeCommandQueue(commandQueue);
}

// TODO: Refactor test
UTEST(Shader, DispatchCompute) 
{
    // Arrange
    InitLog();
    auto graphicsDevice = GetSharedGraphicsDevice();
    
    ElemGraphicsHeapOptions heapOptions =
    {
        .HeapType = ElemGraphicsHeapType_ReadBack
    };

    auto readBackGraphicsHeap = ElemCreateGraphicsHeap(graphicsDevice, 1024, &heapOptions);

    ElemGraphicsResourceInfo bufferInfo =  
    {
        .Type = ElemGraphicsResourceType_Buffer,
        .Width = 64 * sizeof(uint32_t),
        .Usage = ElemGraphicsResourceUsage_Uav
    };

    auto buffer = ElemCreateGraphicsResource(readBackGraphicsHeap, 0, &bufferInfo);

    ElemGraphicsResourceDescriptorInfo descriptorInfo =
    {
        .Resource = buffer,
        .Usage = ElemGraphicsResourceUsage_Uav
    };

    auto bufferWriteDescriptor = ElemCreateGraphicsResourceDescriptor(&descriptorInfo);

    auto shaderLibrary = OpenShader("ShaderTests.shader");
    ASSERT_NE(shaderLibrary, ELEM_HANDLE_NULL);

    ElemComputePipelineStateParameters parameters =
    {
        .ShaderLibrary = shaderLibrary,
        .ComputeShaderFunction = "TestCompute"
    };

    auto pipelineState = ElemCompileComputePipelineState(graphicsDevice, &parameters);
    auto commandQueue = ElemCreateCommandQueue(graphicsDevice, ElemCommandQueueType_Graphics, nullptr);
    auto commandList = ElemGetCommandList(commandQueue, nullptr);
    ElemBindPipelineState(commandList, pipelineState);
    ElemPushPipelineStateConstants(commandList, 0, { .Items = (uint8_t*)&bufferWriteDescriptor, .Length = sizeof(ElemGraphicsResourceDescriptor) });

    // Act
    ElemDispatchCompute(commandList, 1, 1, 1);

    // Assert
    ElemCommitCommandList(commandList);
    auto fence = ElemExecuteCommandList(commandQueue, commandList, nullptr);
    ElemWaitForFenceOnCpu(fence);
    
    ASSERT_FALSE_MSG(testHasLogErrors, testErrorLogs);

    auto bufferData = ElemGetGraphicsResourceDataSpan(buffer);
    auto uintData = (uint32_t*)bufferData.Items;

    for (uint32_t i = 0; i < bufferData.Length / 4; i++)
    {
        printf("Test: %d\n", bufferData.Items[i]);
    }

    ElemFreeGraphicsResourceDescriptor(bufferWriteDescriptor);
    ElemFreeGraphicsResource(buffer);
    ElemFreeCommandQueue(commandQueue);
    ElemFreePipelineState(pipelineState);
}

// TODO: Validate dispatch thread group count
// TODO: Cannot push constant before binding pso
