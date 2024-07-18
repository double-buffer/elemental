struct TestParameters
{
    uint TestDescriptor1;
    uint TestDescriptor2;
    uint ElementCount;
};

[[vk::push_constant]]
TestParameters parameters : register(b0);

[shader("compute")]
[numthreads(16, 1, 1)]
void TestWriteBufferData(uint2 threadId: SV_DispatchThreadID)
{
    if (threadId.x < parameters.ElementCount)
    {
        RWStructuredBuffer<uint> testBuffer = ResourceDescriptorHeap[parameters.TestDescriptor1];
        testBuffer[threadId.x] = threadId.x;
    }
}

[shader("compute")]
[numthreads(16, 1, 1)]
void TestWriteAddBufferData(uint2 threadId: SV_DispatchThreadID)
{
    if (threadId.x < parameters.ElementCount)
    {
        RWStructuredBuffer<uint> testBuffer = ResourceDescriptorHeap[parameters.TestDescriptor1];
        testBuffer[threadId.x] = testBuffer[threadId.x] + parameters.TestDescriptor2;
    }
}

[shader("compute")]
[numthreads(16, 1, 1)]
void TestReadBufferData(uint2 threadId: SV_DispatchThreadID)
{
    if (threadId.x < parameters.ElementCount)
    {
        StructuredBuffer<uint> testBuffer = ResourceDescriptorHeap[parameters.TestDescriptor1];
        RWStructuredBuffer<uint> destinationBuffer = ResourceDescriptorHeap[parameters.TestDescriptor2];
        destinationBuffer[threadId.x] = testBuffer[parameters.ElementCount - threadId.x - 1];
    }
}

[shader("compute")]
[numthreads(16, 16, 1)]
void TestWriteTextureData(uint2 threadId: SV_DispatchThreadID)
{
    RWTexture2D<float4> destinationTexture = ResourceDescriptorHeap[parameters.TestDescriptor1];

    uint width, height;
    destinationTexture.GetDimensions(width, height);
        
    if (threadId.x < width && threadId.y < height)
    {
        destinationTexture[threadId.xy] = float4(threadId, 0.5, 1.0);
    }
}

[shader("compute")]
[numthreads(16, 16, 1)]
void TestReadTextureData(uint2 threadId: SV_DispatchThreadID)
{
    Texture2D<float4> sourceTexture = ResourceDescriptorHeap[parameters.TestDescriptor1];

    uint width, height;
    sourceTexture.GetDimensions(width, height);

    if (threadId.x < width && threadId.y < height)
    {
        RWStructuredBuffer<float4> destinationBuffer = ResourceDescriptorHeap[parameters.TestDescriptor2];
        destinationBuffer[threadId.y * width + threadId.x] = sourceTexture[threadId.xy];
    }
}
