struct ShaderParameters
{
    float RotationY;
};

//[[vk::push_constant]]
ShaderParameters parameters : register(b0);

struct Vertex
{
    float3 Position;
    float4 Color;
};

struct VertexOutput
{
    float4 Position: SV_Position;
    float4 Color: TEXCOORD0;
};

static Vertex triangleVertices[] =
{
    { float3(-0.5, 0.5, 0.0), float4(1.0, 0.0, 0.0, 1.0) },
    { float3(0.5, 0.5, 0.0), float4(0.0, 1.0, 0.0, 1.0) },
    { float3(-0.5, -0.5, 0.0), float4(0.0, 0.0, 1.0, 1.0) }
};

float3 Transform(float3 position, float rotationY) 
{
    float cosY = cos(rotationY);
    float sinY = sin(rotationY);
    return float3(position.x * cosY - position.z * sinY, position.y, position.x * sinY + position.z * cosY + 0.5);
}

// Test
[shader("intersection")]
void InterTest()
{
}

// Test
[shader("compute")]
[NumThreads(32, 1, 1)]
void CSMain()
{
}

[shader("mesh")]
[OutputTopology("triangle")]
[NumThreads(32, 1, 1)]
void MeshMain(in uint groupThreadId : SV_GroupThreadID, out vertices VertexOutput vertices[3], out indices uint3 indices[1])
{
    const uint meshVertexCount = 3;

    SetMeshOutputCounts(meshVertexCount, 1);

    if (groupThreadId < meshVertexCount)
    {
        vertices[groupThreadId].Position = float4(Transform(triangleVertices[groupThreadId].Position, parameters.RotationY), 1.0);
        vertices[groupThreadId].Color = triangleVertices[groupThreadId].Color;
    }

    if (groupThreadId == 0)
    {
        indices[groupThreadId] = uint3(0, 1, 2);
    }
}

[shader("pixel")]
float4 PixelMain(const VertexOutput input) : SV_Target0
{
    return input.Color; 
}
