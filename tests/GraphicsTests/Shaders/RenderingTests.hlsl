struct Parameters
{
    uint SourceBufferIndex;
    uint DestinationBufferIndex;
};

[[vk::push_constant]]
Parameters parameters : register(b0);

[shader("compute")]
[numthreads(16, 16, 1)]
void CopyTexture(uint2 threadId: SV_DispatchThreadID)
{
    Texture2D<float4> sourceTexture = ResourceDescriptorHeap[parameters.SourceBufferIndex];
    RWStructuredBuffer<float4> destinationBuffer = ResourceDescriptorHeap[parameters.DestinationBufferIndex];
    
    destinationBuffer[threadId.y * 16 + threadId.x] = sourceTexture.Load(uint3(threadId, 0));
}
