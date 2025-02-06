struct ShaderParameters
{
    uint32_t RenderTextureIndex;
    uint32_t SampleCount;
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

float3 ApplyExposure(float3 color, float exposure)
{
    color.rgb *= exp2(exposure);
    return color;
}

float3 ToneMapACESFitted(float3 color)
{
    color = (color * (2.51 * color + 0.03)) / (color * (2.43 * color + 0.59) + 0.14);
    return saturate(color);
}

float3 AdjustSaturation(float3 color, float saturationAdjust)
{
    float lum = dot(color, float3(0.299, 0.587, 0.114));
    float3 grey = float3(lum, lum, lum);
    color = lerp(grey, color, saturationAdjust);

    return color;
}

[shader("pixel")]
float4 PixelMain(const VertexOutput input) : SV_Target0
{
    float exposure = -5.0;
    float saturationAdjust = 0.85;

    Texture2D<float4> renderTexture = ResourceDescriptorHeap[parameters.RenderTextureIndex];
    float4 sourceColor = renderTexture.Load(int3(input.Position.xy, 0));

    //return sourceColor;

    float3 color = sourceColor.rgb / parameters.SampleCount;

    color = ApplyExposure(color, exposure);
    color = ToneMapACESFitted(color);
    color = AdjustSaturation(color, saturationAdjust);

    return float4(color, sourceColor.a);
}
