struct ShaderParameters
{
    uint VertexBufferIndex;
    uint IndexBufferIndex;
    uint VertexOffset;
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
    float4 row2 = float4(0.0f, 2.0f / (minPlaneY - maxPlaneY), 0.0f, 0.0f);
    float4 row3 = float4(0.0f, 0.0f, 0.5, 1.0f);
    float4 row4 = float4((minPlaneX + maxPlaneX) / (minPlaneX - maxPlaneX), (minPlaneY + maxPlaneY) / (maxPlaneY - minPlaneY), 0.5, 1.0f);

    return float4x4(row1, row2, row3, row4);
}

#define MAX_VERTEX_COUNT 126

[shader("mesh")]
[OutputTopology("triangle")]
[NumThreads(MAX_VERTEX_COUNT, 1, 1)]
void MeshMain(in uint groupId: SV_GroupID, 
              in uint groupThreadId : SV_GroupThreadID, 
              out vertices VertexOutput vertices[MAX_VERTEX_COUNT], 
              out indices uint3 indices[MAX_VERTEX_COUNT / 3])
{
    // TODO: For the moment we process several time the same vertices
    uint vertexCount = min(MAX_VERTEX_COUNT, parameters.IndexCount - groupId * MAX_VERTEX_COUNT);
    uint triangleCount = vertexCount / 3;
    SetMeshOutputCounts(vertexCount, triangleCount);

    if (groupThreadId < vertexCount)
    {
        float4x4 projectionMatrix = CreateOrthographicMatrixOffCenter(0, parameters.RenderWidth, 0, parameters.RenderHeight);

        ByteAddressBuffer indexBuffer = ResourceDescriptorHeap[parameters.IndexBufferIndex];
        ByteAddressBuffer vertexBuffer = ResourceDescriptorHeap[parameters.VertexBufferIndex];

        uint16_t vertexIndex = indexBuffer.Load<uint16_t>((parameters.IndexOffset + groupId * MAX_VERTEX_COUNT + groupThreadId) * sizeof(uint16_t));
        ImDrawVert vertex = vertexBuffer.Load<ImDrawVert>((parameters.VertexOffset + vertexIndex) * sizeof(ImDrawVert));

        float4 color = unpack_u8u32(vertex.Color) / 255.0;
        color = float4(pow(color.rgb, 2.2f), color.a);

        vertices[groupThreadId].Position = mul(float4(vertex.Position.x, vertex.Position.y, 0.0, 1.0), projectionMatrix);
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
