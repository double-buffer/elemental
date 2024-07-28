struct ShaderParameters
{
    uint32_t RenderTextureIndex;
};

[[vk::push_constant]]
ShaderParameters parameters : register(b0);

struct Vertex
{
    float4 Position;
    float2 TextureCoordinates;
};

struct VertexOutput
{
    float4 Position: SV_Position;
    float2 TextureCoordinates: TEXCOORD0;
};

static Vertex quadVertices[] =
{
    { float4(-1.0, 1.0, 0.0, 1.0), float2(0.0, 0.0) },
    { float4(1.0, 1.0, 0.0, 1.0), float2(1.0, 0.0) },
    { float4(-1.0, -1.0, 0.0, 1.0), float2(0.0, 1.0) },
    { float4(1.0, -1.0, 0.0, 1.0), float2(1.0, 1.0) }
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
    const uint meshVertexCount = 4;
    const uint triangleCount = 2;

    SetMeshOutputCounts(meshVertexCount, triangleCount);

    if (groupThreadId < meshVertexCount)
    {
        vertices[groupThreadId].Position = quadVertices[groupThreadId].Position;
        vertices[groupThreadId].TextureCoordinates = quadVertices[groupThreadId].TextureCoordinates;
    }

    if (groupThreadId < triangleCount)
    {
        indices[groupThreadId] = quadIndices[groupThreadId];
    }
}

[shader("pixel")]
float4 PixelMain(const VertexOutput input) : SV_Target0
{
    Texture2D<float4> renderTexture = ResourceDescriptorHeap[parameters.RenderTextureIndex];
    return renderTexture.Load(int3(input.Position.xy, 0));
}
