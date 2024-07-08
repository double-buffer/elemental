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
