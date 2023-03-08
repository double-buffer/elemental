struct VertexOutput
{
    float4 Position: SV_Position;
    float4 Color: TEXCOORD0;
};

static float4 triangleVertices[] =
{
    float4(-0.5, 0.5, 0, 1),
    float4(0.5, 0.5, 0, 1),
    float4(-0.5, -0.5, 0, 1)
};

static float4 triangleColors[] =
{
    float4(1.0, 0.0, 0, 1),
    float4(0.0, 1.0, 0, 1),
    float4(0.0, 0.0, 1, 1)
};

static uint3 rectangleIndices[] =
{
    uint3(0, 1, 2)
};

[OutputTopology("triangle")]
[NumThreads(32, 1, 1)]
void MeshMain(in uint groupId : SV_GroupID, in uint groupThreadId : SV_GroupThreadID, out vertices VertexOutput vertices[128], out indices uint3 indices[128])
{
    const uint meshVertexCount = 3;
    const uint meshPrimitiveCount = 1;

    SetMeshOutputCounts(meshVertexCount, meshPrimitiveCount);

    if (groupThreadId < meshVertexCount)
    {
        vertices[groupThreadId].Position = triangleVertices[groupThreadId];
        vertices[groupThreadId].Color = triangleColors[groupThreadId];
    }

    if (groupThreadId < meshPrimitiveCount)
    {
        indices[groupThreadId] = rectangleIndices[groupThreadId];
    }
}

struct PixelOutput
{
    float4 Color: SV_TARGET0;
};

PixelOutput PixelMain(const VertexOutput input)
{
    PixelOutput output = (PixelOutput)0;

    output.Color = input.Color;
    
    return output; 
}