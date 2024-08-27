struct ShaderParameters
{
    uint VertexBufferIndex;
    uint IndexBufferIndex;
    uint VertexOffset;
    uint VertexCount;
    uint IndexOffset;
    uint IndexCount;
    uint RenderWidth;
    uint RenderHeight;
};

[[vk::push_constant]]
ShaderParameters parameters : register(b0);

struct ImDrawVert
{
    float2 Position;
    float2 TextureCoordinates;
    uint Color;
};

struct VertexOutput
{
    float4 Position: SV_Position;
};

[shader("mesh")]
[OutputTopology("triangle")]
[NumThreads(128, 1, 1)]
void MeshMain(in uint groupId: SV_GroupID, 
              in uint groupThreadId : SV_GroupThreadID, 
              out vertices VertexOutput vertices[64], 
              out indices uint3 indices[128])
{
    // TODO: Compute correct count
    uint vertexCount = 4;//parameters.VertexCount / 64;
    uint triangleCount = 1;//parameters.IndexCount / 3 / 64;
    SetMeshOutputCounts(parameters.VertexCount, triangleCount);

    if (groupThreadId < vertexCount)
    {
        ByteAddressBuffer indexBuffer = ResourceDescriptorHeap[parameters.IndexBufferIndex];
        ByteAddressBuffer vertexBuffer = ResourceDescriptorHeap[parameters.VertexBufferIndex];

        uint vertexIndex = indexBuffer.Load<uint16_t>((parameters.IndexOffset + groupThreadId) * sizeof(uint16_t));
        ImDrawVert vertex = vertexBuffer.Load<ImDrawVert>(parameters.VertexOffset + vertexIndex * sizeof(ImDrawVert));

        vertices[groupThreadId].Position = float4(vertex.Position.x / parameters.RenderWidth, vertex.Position.y / parameters.RenderHeight, 0.0, 1.0);
    }

    if (groupThreadId < triangleCount)
    {
        indices[groupThreadId] = uint3(groupThreadId * 3, groupThreadId * 3 + 1, groupThreadId * 3 + 2);
    }
}

[shader("pixel")]
float4 PixelMain(const VertexOutput input) : SV_Target0
{
    return float4(1.0, 1.0, 0.0, 1.0);
}
