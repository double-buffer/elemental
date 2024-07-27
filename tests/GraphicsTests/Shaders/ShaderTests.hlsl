struct TestParameters
{
    uint TestDescriptor;
};

[[vk::push_constant]]
TestParameters parameters : register(b0);

[shader("compute")]
[numthreads(16, 1, 1)]
void TestCompute(uint2 threadId: SV_DispatchThreadID)
{
    RWStructuredBuffer<uint> testBuffer = ResourceDescriptorHeap[parameters.TestDescriptor];
    testBuffer[threadId.x] = threadId.x;
}
