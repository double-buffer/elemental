#include "Elemental.h"
#include "ElementalTools.h"
#include "GraphicsTests.h"
#include "utest.h"

// TODO: For the moment graphics tests will not run on mobile platform because we need in that case to compile test shaders separately

// TODO: For now we use 16x16 to make it pass on metal but it should be 16x1
auto testComputeShader = R"(
    [[vk::push_constant]]
    uint testBufferIndex : register(b0);

    [shader("compute")]
    [numthreads(16, 16, 1)]
    void TestCompute(uint threadId: SV_DispatchThreadID)
    {
        RWStructuredBuffer<float> testBuffer = ResourceDescriptorHeap[testBufferIndex];
        testBuffer[threadId] = threadId;
    }
)";

ElemShaderLibrary TestCompileShader(const char* shaderSource)
{
    auto graphicsDevice = GetSharedGraphicsDevice();
    auto systemInfo = GetSharedSystemInfo();
    auto graphicsDeviceInfo = GetSharedGraphicsDeviceInfo();

    ElemShaderSourceData sourceData = 
    { 
        .ShaderLanguage = ElemShaderLanguage_Hlsl,
        .Data = 
        { 
            .Items = (uint8_t*)shaderSource, 
            .Length = (uint32_t)strlen(shaderSource) 
        } 
    };

    auto compiledShader = ElemCompileShaderLibrary((ElemToolsGraphicsApi)graphicsDeviceInfo.GraphicsApi, (ElemToolsPlatform)systemInfo.Platform, &sourceData, nullptr);

    for (uint32_t i = 0; i < compiledShader.Messages.Length; i++)
    {
        printf("Compil msg (%d): %s\n", compiledShader.Messages.Items[i].Type, compiledShader.Messages.Items[i].Message);
    }

    if (compiledShader.HasErrors)
    {
        printf("Error while compiling shader!\n");
        return ELEM_HANDLE_NULL;
    }

    auto shaderLibrary = ElemCreateShaderLibrary(graphicsDevice, { .Items = compiledShader.Data.Items, .Length = compiledShader.Data.Length });
    return shaderLibrary;
}

UTEST(Shader, CreateComputePipelineState) 
{
    // Arrange
    InitLog();
    auto graphicsDevice = GetSharedGraphicsDevice();
    auto shaderLibrary = TestCompileShader(testComputeShader);
    ASSERT_NE(shaderLibrary, ELEM_HANDLE_NULL);

    // Act
    ElemComputePipelineStateParameters parameters =
    {
        .ShaderLibrary = shaderLibrary,
        .ComputeShaderFunction = "TestCompute"
    };

    auto pipelineState = ElemCompileComputePipelineState(graphicsDevice, &parameters); 

    // Assert
    ASSERT_NE(pipelineState, ELEM_HANDLE_NULL);
    ASSERT_FALSE(testHasLogErrors);

    ElemFreePipelineState(pipelineState);
}

UTEST(Shader, CreateComputePipelineStateFunctionNotExist) 
{
    // Arrange
    InitLog();
    auto graphicsDevice = GetSharedGraphicsDevice();
    auto shaderLibrary = TestCompileShader(testComputeShader);
    ASSERT_NE(shaderLibrary, ELEM_HANDLE_NULL);

    // Act
    ElemComputePipelineStateParameters parameters =
    {
        .ShaderLibrary = shaderLibrary,
        .ComputeShaderFunction = "TestComputeNotExist"
    };

    auto pipelineState = ElemCompileComputePipelineState(graphicsDevice, &parameters); 

    // Assert
    ASSERT_EQ(pipelineState, ELEM_HANDLE_NULL);
    ASSERT_TRUE(testHasLogErrors);
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
    ASSERT_TRUE(testHasLogErrors);
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
        .Width = 64,
        .Usage = ElemGraphicsResourceUsage_Uav
    };

    auto buffer = ElemCreateGraphicsResource(readBackGraphicsHeap, 0, &bufferInfo);

    ElemGraphicsResourceDescriptorInfo descriptorInfo =
    {
        .Resource = buffer,
        .Usage = ElemGraphicsResourceUsage_Uav
    };

    auto bufferWriteDescriptor = ElemCreateGraphicsResourceDescriptor(&descriptorInfo);

    auto shaderLibrary = TestCompileShader(testComputeShader);
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

    auto bufferData = ElemGetGraphicsResourceDataSpan(buffer);

    for (uint32_t i = 0; i < bufferData.Length; i++)
    {
        printf("Test: %d\n", bufferData.Items[i]);
    }

    ASSERT_FALSE(testHasLogErrors);

    ElemFreeGraphicsResourceDescriptor(bufferWriteDescriptor);
    ElemFreeGraphicsResource(buffer);
    ElemFreeCommandQueue(commandQueue);
    ElemFreePipelineState(pipelineState);
}

// TODO: Validate dispatch thread group count
// TODO: Cannot push constant before binding pso
