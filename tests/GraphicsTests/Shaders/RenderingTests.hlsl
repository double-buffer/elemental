struct VertexOutput
{
    float4 Position: SV_Position;
};

static float3 rectangleVertices[] =
{
    float3(-1.0, 1.0, 0.0),
    float3(1.0, 1.0, 0.0),
    float3(-1.0, -1.0, 0.0),
    float3(1.0, -1.0, 0.0)
};

static uint3 rectanglePrimitives[] =
{
    uint3(0, 1, 2),
    uint3(2, 1, 3)
};

[shader("mesh")]
[OutputTopology("triangle")]
[NumThreads(4, 1, 1)]
void MeshShader(in uint groupThreadId : SV_GroupThreadID, out vertices VertexOutput vertices[4], out indices uint3 indices[2])
{
    const uint meshVertexCount = 4;
    const uint meshPrimitiveCount = 2;

    SetMeshOutputCounts(meshVertexCount, meshPrimitiveCount);

    if (groupThreadId < meshVertexCount)
    {
        vertices[groupThreadId].Position = float4(rectangleVertices[groupThreadId], 1);
    }

    if (groupThreadId < meshPrimitiveCount)
    {
        indices[groupThreadId] = rectanglePrimitives[groupThreadId];
    }
}

[shader("pixel")]
float4 PixelShader(const VertexOutput input) : SV_Target0
{
    return float4(1.0, 1.0, 0.0, 1.0); 
}
