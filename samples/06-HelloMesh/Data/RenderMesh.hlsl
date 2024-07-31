struct ShaderParameters
{
    uint32_t VertexBufferIndex;
};

[[vk::push_constant]]
ShaderParameters parameters : register(b0);

struct VertexOutput
{
    float4 Position: SV_Position;
};

static uint3 quadIndices[] =
{
    uint3(0, 1, 2),
    uint3(2, 1, 3)
};

[shader("mesh")]
[OutputTopology("triangle")]
[NumThreads(32, 1, 1)]
void MeshMain(in uint groupThreadId : SV_GroupThreadID, out vertices VertexOutput vertices[4], out indices uint3 indices[2])
{
    const uint meshVertexCount = 3;
    const uint triangleCount = 1;

    SetMeshOutputCounts(meshVertexCount, triangleCount);

    if (groupThreadId < meshVertexCount)
    {
        ByteAddressBuffer vertexBuffer = ResourceDescriptorHeap[parameters.VertexBufferIndex];
        vertices[groupThreadId].Position = float4(vertexBuffer.Load<float3>(groupThreadId * sizeof(float3)), 1.0);
    }

    if (groupThreadId < triangleCount)
    {
        indices[groupThreadId] = quadIndices[groupThreadId];
    }
}

[shader("pixel")]
float4 PixelMain(const VertexOutput input) : SV_Target0
{
    return float4(1.0, 1.0, 0.0, 1.0);
}
