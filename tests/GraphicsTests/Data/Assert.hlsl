struct Parameters
{
    uint SourceBufferIndex;
    uint DestinationBufferIndex;
    uint MipLevel;
};

[[vk::push_constant]]
Parameters parameters : register(b0);

[shader("compute")]
[numthreads(8, 8, 1)]
void CopyTexture(uint2 threadId: SV_DispatchThreadID)
{
    Texture2D<float4> sourceTexture = ResourceDescriptorHeap[parameters.SourceBufferIndex];
    RWStructuredBuffer<float4> destinationBuffer = ResourceDescriptorHeap[parameters.DestinationBufferIndex];
    
    float width;
    float height;
    float mipLevelCount;
    sourceTexture.GetDimensions(parameters.MipLevel, width, height, mipLevelCount);

    destinationBuffer[threadId.y * width + threadId.x] = sourceTexture.Load(uint3(threadId, parameters.MipLevel));
}

[shader("compute")]
[numthreads(8, 8, 1)]
void CopyTextureFloat(uint2 threadId: SV_DispatchThreadID)
{
    Texture2D<float> sourceTexture = ResourceDescriptorHeap[parameters.SourceBufferIndex];
    RWStructuredBuffer<float> destinationBuffer = ResourceDescriptorHeap[parameters.DestinationBufferIndex];
    
    float width;
    float height;
    float mipLevelCount;
    sourceTexture.GetDimensions(parameters.MipLevel, width, height, mipLevelCount);

    destinationBuffer[threadId.y * width + threadId.x] = sourceTexture.Load(uint3(threadId, parameters.MipLevel));
}
