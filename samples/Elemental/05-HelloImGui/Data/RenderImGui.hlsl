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
    float2 TextureCoordinates: TEXCOORD0;
    float4 Color: COLOR0;
};

float4x4 CreateOrthographicMatrixOffCenter(float minPlaneX, float maxPlaneX, float minPlaneY, float maxPlaneY)
{
    float4 row1 = float4(2.0f / (maxPlaneX - minPlaneX), 0.0f, 0.0f, 0.0f);
    float4 row2 = float4(0.0f, 2.0f / (maxPlaneY - minPlaneY), 0.0f, 0.0f);
    float4 row3 = float4(0.0f, 0.0f, 0.5, 1.0f);
    float4 row4 = float4((minPlaneX + maxPlaneX) / (minPlaneX - maxPlaneX), (minPlaneY + maxPlaneY) / (minPlaneY - maxPlaneY), 0.5, 1.0f);

    return float4x4(row1, row2, row3, row4);
}

[shader("mesh")]
[OutputTopology("triangle")]
[NumThreads(126, 1, 1)]
void MeshMain(in uint groupId: SV_GroupID, 
              in uint groupThreadId : SV_GroupThreadID, 
              out vertices VertexOutput vertices[126], 
              out indices uint3 indices[64])
{
    // TODO: Compute correct count
    uint vertexCount = 126;//parameters.VertexCount / 64;
    uint triangleCount = 42;//parameters.IndexCount / 3 / 64;
    SetMeshOutputCounts(vertexCount, triangleCount);

    if (groupThreadId < vertexCount)
    {
        float4x4 projectionMatrix = CreateOrthographicMatrixOffCenter(0, parameters.RenderWidth, parameters.RenderHeight, 0);

        ByteAddressBuffer indexBuffer = ResourceDescriptorHeap[parameters.IndexBufferIndex];
        ByteAddressBuffer vertexBuffer = ResourceDescriptorHeap[parameters.VertexBufferIndex];

        uint16_t vertexIndex = indexBuffer.Load<uint16_t>((parameters.IndexOffset + groupId * 126 + groupThreadId) * sizeof(uint16_t));
        ImDrawVert vertex = vertexBuffer.Load<ImDrawVert>((parameters.VertexOffset + vertexIndex) * sizeof(ImDrawVert));

        float4 output = mul(float4(vertex.Position.x, vertex.Position.y, 0.0, 1.0), projectionMatrix);
        float4 color = unpack_u8u32(vertex.Color) / 255.0;
        color.rgb *= color.a;
        color = float4(pow(abs(color.rgb), 2.2f), 1.0 - pow(abs(1.0 - color.a), 2.2f));
        //color = pow(abs(color), 2.2f);

        vertices[groupThreadId].Position = output;
        vertices[groupThreadId].TextureCoordinates = vertex.TextureCoordinates;
        vertices[groupThreadId].Color = color;
    }

    if (groupThreadId < triangleCount)
    {
        indices[groupThreadId] = uint3(groupThreadId * 3, groupThreadId * 3 + 1, groupThreadId * 3 + 2);
    }
}

[shader("pixel")]
float4 PixelMain(const VertexOutput input) : SV_Target0
{
    return input.Color;
}
