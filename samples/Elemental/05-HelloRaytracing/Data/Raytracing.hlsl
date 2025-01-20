struct ShaderParameters
{
    uint32_t AccelerationStructureIndex;
    uint32_t FrameDataBufferIndex;
};

[[vk::push_constant]]
ShaderParameters parameters : register(b0);

struct FrameData
{
    float4x4 ViewProjMatrix;
    float4x4 InverseViewMatrix;
    float4x4 InverseProjectionMatrix;
    uint32_t ShowMeshlets;
};

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
    { float4(-1.0, 1.0, 0.0, 1.0), float2(0.0, 1.0) },
    { float4(1.0, 1.0, 0.0, 1.0), float2(1.0, 1.0) },
    { float4(-1.0, -1.0, 0.0, 1.0), float2(0.0, 0.0) },
    { float4(1.0, -1.0, 0.0, 1.0), float2(1.0, 0.0) }
};

static uint3 quadIndices[] =
{
    uint3(0, 1, 2),
    uint3(2, 1, 3)
};

[shader("mesh")]
[OutputTopology("triangle")]
[NumThreads(4, 1, 1)]
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

uint hash(uint a)
{
   a = (a+0x7ed55d16) + (a<<12);
   a = (a^0xc761c23c) ^ (a>>19);
   a = (a+0x165667b1) + (a<<5);
   a = (a+0xd3a2646c) ^ (a<<9);
   a = (a+0xfd7046c5) + (a<<3);
   a = (a^0xb55a4f09) ^ (a>>16);

   return a;
}

[shader("pixel")]
float4 PixelMain(const VertexOutput input) : SV_Target0
{
    ByteAddressBuffer frameDataBuffer = ResourceDescriptorHeap[parameters.FrameDataBufferIndex];
    FrameData frameData = frameDataBuffer.Load<FrameData>(0);

    float4 targetClipSpace = float4(input.TextureCoordinates.xy * 2.0 - 1.0, 1.0, 1.0);
    float4 targetViewSpace = mul(targetClipSpace, frameData.InverseProjectionMatrix);
    targetViewSpace.xyz /= targetViewSpace.w;

    float3 rayOriginViewSpace = float3(0.0, 0.0, 0.0);
    float3 rayDirectionViewSpace = normalize(targetViewSpace);

    float3 rayOriginWorldSpace = mul(float4(rayOriginViewSpace, 1), frameData.InverseViewMatrix).xyz;
    float3 rayDirectionWorldSpace = mul(float4(rayDirectionViewSpace, 0), frameData.InverseViewMatrix).xyz;
    rayDirectionWorldSpace = normalize(rayDirectionWorldSpace);

    RaytracingAccelerationStructure raytracingAccelerationStructure = ResourceDescriptorHeap[parameters.AccelerationStructureIndex];

    RayQuery<RAY_FLAG_NONE> rayQuery;

    RayDesc ray;
	ray.Origin = rayOriginWorldSpace;
	ray.TMin = 0.0;
	ray.TMax = 10000.0;
	ray.Direction = rayDirectionWorldSpace;

    rayQuery.TraceRayInline(raytracingAccelerationStructure, RAY_FLAG_NONE, 0xFF, ray);

    while (rayQuery.Proceed());

    if (rayQuery.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
    {
        float distance = rayQuery.CommittedRayT() * 0.01;
        uint instanceId = rayQuery.CommittedInstanceID();
        uint primitiveIndex = rayQuery.CommittedPrimitiveIndex();
        uint geometryIndex = rayQuery.CommittedGeometryIndex();
    
        /*
        ByteAddressBuffer materialBuffer = ResourceDescriptorHeap[parameters.MaterialBuffer];
        ShaderMaterial material = materialBuffer.Load<ShaderMaterial>(parameters.MaterialId * sizeof(ShaderMaterial));

        float4 albedo = material.AlbedoFactor;
        float3 worldNormal = normalize(input.WorldNormal);
        float nDotL = max(dot(worldNormal, normalize(float3(1.0, 1.0, -1.0))), 0.0);
        float ambient = 0.05;

        return albedo * (nDotL + ambient);*/

        uint hashResult = hash(instanceId);
        float3 hashColor = float3(float(hashResult & 255), float((hashResult >> 8) & 255), float((hashResult >> 16) & 255)) / 255.0;
        return float4(hashColor, 1);

        return float4(distance, distance, distance, 1);
    }
    else
    {
        discard;
    }

    return float4(1, 1, 0, 1);
}
