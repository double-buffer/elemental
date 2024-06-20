[[vk::push_constant]]
uint testBufferIndex : register(b0);

[shader("compute")]
[numthreads(16, 1, 1)]
void TestCompute(uint2 threadId: SV_DispatchThreadID)
{
    RWStructuredBuffer<uint> testBuffer = ResourceDescriptorHeap[testBufferIndex];
    testBuffer[threadId.x] = threadId.x;
}
