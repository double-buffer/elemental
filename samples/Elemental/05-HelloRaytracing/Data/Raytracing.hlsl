struct ShaderParameters
{
    uint32_t AccelerationStructureIndex;
    uint32_t GlobalParametersBufferIndex;
};

[[vk::push_constant]]
ShaderParameters parameters : register(b0);

// TODO: Reorder parameters
struct GlobalParameters
{
    float4x4 ViewProjMatrix;
    float4x4 InverseViewMatrix;
    float4x4 InverseProjectionMatrix;
    uint32_t MaterialBufferIndex;
    uint32_t GpuMeshInstanceBufferIndex;
    uint32_t GpuMeshPrimitiveInstanceBufferIndex;
    uint32_t Action; // TODO: One bit per action
};

struct GpuMeshPrimitive
{
    uint32_t MeshletOffset;
    uint32_t MeshletCount;
    uint32_t VertexBufferOffset;
    uint32_t VertexCount;
    uint32_t IndexBufferOffset;
    uint32_t IndexCount;
    int32_t MaterialId;
    // TODO: BoundingBox
};

struct GpuMeshInstance
{
    int32_t MeshBufferIndex;
    float4 Rotation;
    float3 Translation;
    float Scale;
};

struct GpuMeshPrimitiveInstance
{
    int32_t MeshInstanceId;
    int32_t MeshPrimitiveId;
};

struct GpuDrawParameters
{
    ByteAddressBuffer MeshBuffer;
    int32_t MaterialId;
    uint32_t VertexBufferOffset;
    uint32_t IndexBufferOffset;
    uint32_t MeshletOffset;
    float4 Rotation;
    float3 Translation;
    float Scale;
};

struct MeshVertex
{
    float3 Position;
    float3 Normal;
    float4 Tangent;
    float2 TextureCoordinates;
};

typedef struct
{
    int32_t AlbedoTextureId;
    int32_t NormalTextureId;
    float4 AlbedoFactor;
} ShaderMaterial;

// TODO: Do other functions if we need less indirection
GpuDrawParameters GetDrawParametersNonUniform(GlobalParameters globalParameters, int32_t meshPrimitiveInstanceId)
{
    ByteAddressBuffer meshInstanceBuffer = ResourceDescriptorHeap[globalParameters.GpuMeshInstanceBufferIndex];
    ByteAddressBuffer meshPrimitiveInstanceBuffer = ResourceDescriptorHeap[globalParameters.GpuMeshPrimitiveInstanceBufferIndex];

    GpuMeshPrimitiveInstance meshPrimitiveInstance = meshPrimitiveInstanceBuffer.Load<GpuMeshPrimitiveInstance>(meshPrimitiveInstanceId * sizeof(GpuMeshPrimitiveInstance));

    GpuMeshInstance meshInstance = meshInstanceBuffer.Load<GpuMeshInstance>(meshPrimitiveInstance.MeshInstanceId * sizeof(GpuMeshInstance));
    // TODO: Be carreful with uniform/nonuniform (normally it should be uniform here because we process one primitive per meshlet but for raytracing sometimes it isn't)
    ByteAddressBuffer meshBuffer = ResourceDescriptorHeap[NonUniformResourceIndex(meshInstance.MeshBufferIndex)];

    GpuMeshPrimitive meshPrimitive = meshBuffer.Load<GpuMeshPrimitive>(meshPrimitiveInstance.MeshPrimitiveId * sizeof(GpuMeshPrimitive));

    GpuDrawParameters result;
    result.MeshBuffer = meshBuffer;
    result.MaterialId = meshPrimitive.MaterialId;
    result.VertexBufferOffset = meshPrimitive.VertexBufferOffset;
    result.IndexBufferOffset = meshPrimitive.IndexBufferOffset;
    result.MeshletOffset = meshPrimitive.MeshletOffset;
    result.Rotation = meshInstance.Rotation;
    result.Translation = meshInstance.Translation;
    result.Scale = meshInstance.Scale;

    return result;
}


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

float3 RotateQuaternion(float3 v, float4 q)
{
	return v + 2.0 * cross(q.xyz, cross(q.xyz, v) + q.w * v);
}

[shader("pixel")]
float4 PixelMain(const VertexOutput input) : SV_Target0
{
    ByteAddressBuffer globalParametersBuffer = ResourceDescriptorHeap[parameters.GlobalParametersBufferIndex];
    GlobalParameters globalParameters = globalParametersBuffer.Load<GlobalParameters>(0);

    float4 targetClipSpace = float4(input.TextureCoordinates.xy * 2.0 - 1.0, 1.0, 1.0);
    float4 targetViewSpace = mul(targetClipSpace, globalParameters.InverseProjectionMatrix);
    targetViewSpace.xyz /= targetViewSpace.w;

    float3 rayOriginViewSpace = float3(0.0, 0.0, 0.0);
    float3 rayDirectionViewSpace = normalize(targetViewSpace);

    float3 rayOriginWorldSpace = mul(float4(rayOriginViewSpace, 1), globalParameters.InverseViewMatrix).xyz;
    float3 rayDirectionWorldSpace = mul(float4(rayDirectionViewSpace, 0), globalParameters.InverseViewMatrix).xyz;
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
        uint instanceId = rayQuery.CommittedInstanceID();
        uint primitiveIndex = rayQuery.CommittedPrimitiveIndex();
        float2 bary = rayQuery.CommittedTriangleBarycentrics();

        GpuDrawParameters drawParameters = GetDrawParametersNonUniform(globalParameters, instanceId);
        
        uint3 indices = drawParameters.MeshBuffer.Load<uint3>(drawParameters.IndexBufferOffset + primitiveIndex * sizeof(uint3));

        MeshVertex vertexA = drawParameters.MeshBuffer.Load<MeshVertex>(drawParameters.VertexBufferOffset + indices.x * sizeof(MeshVertex));
        MeshVertex vertexB = drawParameters.MeshBuffer.Load<MeshVertex>(drawParameters.VertexBufferOffset + indices.y * sizeof(MeshVertex));
        MeshVertex vertexC = drawParameters.MeshBuffer.Load<MeshVertex>(drawParameters.VertexBufferOffset + indices.z * sizeof(MeshVertex));

        float3 hitPosition = vertexA.Position * (1 - bary.x - bary.y) + vertexB.Position * bary.x + vertexC.Position * bary.y;
        float3 hitNormal = normalize(vertexA.Normal * (1 - bary.x - bary.y) + vertexB.Normal * bary.x + vertexC.Normal * bary.y);

        float3 worldPosition = RotateQuaternion(hitPosition, drawParameters.Rotation) * drawParameters.Scale + drawParameters.Translation;
        float3 worldNormal = RotateQuaternion(hitNormal, drawParameters.Rotation);
    
        ByteAddressBuffer materialBuffer = ResourceDescriptorHeap[globalParameters.MaterialBufferIndex];
        ShaderMaterial material = materialBuffer.Load<ShaderMaterial>(drawParameters.MaterialId * sizeof(ShaderMaterial));
    
        float4 albedo = material.AlbedoFactor;
        float nDotL = max(dot(worldNormal, normalize(float3(1.0, 1.0, -1.0))), 0.0);
        float ambient = 0.05;
        float shadowContribution = 1.0;

        RayQuery<RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH> shadowRayQuery;

        RayDesc shadowRay;
        shadowRay.Origin = worldPosition;
        shadowRay.TMin = 0.000001;
        shadowRay.TMax = 10000.0;
        shadowRay.Direction = normalize(float3(1.0, 1.0, -1.0));

        shadowRayQuery.TraceRayInline(raytracingAccelerationStructure, RAY_FLAG_NONE, 0xFF, shadowRay);
        shadowRayQuery.Proceed();

        if(shadowRayQuery.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
        {
            shadowContribution = 0.0;
        }

        return albedo * (nDotL  * shadowContribution+ ambient);
    }
    else
    {
        discard;
    }

    return float4(1, 1, 0, 1);
}
