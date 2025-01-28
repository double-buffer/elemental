struct ShaderParameters
{
    uint32_t AccelerationStructureIndex;
    uint32_t GlobalParametersBufferIndex;
    uint32_t FrameIndex;
    uint32_t PathTraceLength;
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

typedef struct
{
    int32_t AlbedoTextureId;
    int32_t NormalTextureId;
    float4 AlbedoFactor;
    float3 EmissiveFactor;
} ShaderMaterial;

struct GpuDrawParameters
{
    ByteAddressBuffer MeshBuffer;
    ShaderMaterial Material;
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

struct GlobalShaderData
{
    ByteAddressBuffer MeshInstanceBuffer;
    ByteAddressBuffer MeshPrimitiveInstanceBuffer;
    ByteAddressBuffer MaterialBuffer;
    RaytracingAccelerationStructure RaytracingAccelerationStructure;
    float4x4 ViewProjMatrix;
    float4x4 InverseViewMatrix;
    float4x4 InverseProjectionMatrix;
    uint32_t Action;
};

#define M_PI 3.141592653589793238462643

// Random number generation
//------------------------------------------------------------------------------------------------------
// From: http://intro-to-dxr.cwyman.org/
uint initRand(uint val0, uint val1, uint backoff = 16)
{
  uint v0 = val0, v1 = val1, s0 = 0;

  [unroll]
  for (uint n = 0; n < backoff; n++)
  {
    s0 += 0x9e3779b9;
    v0 += ((v1 << 4) + 0xa341316c) ^ (v1 + s0) ^ ((v1 >> 5) + 0xc8013ea4);
    v1 += ((v0 << 4) + 0xad90777d) ^ (v0 + s0) ^ ((v0 >> 5) + 0x7e95761e);
  }
  return v0;
}

//------------------------------------------------------------------------------------------------------
// From: http://intro-to-dxr.cwyman.org/
// Takes a seed, updates it, and returns a pseudorandom float in [0..1]
float nextRand(inout uint s)
{
  s = (1664525u * s + 1013904223u);
  return float(s & 0x00FFFFFF) / float(0x01000000);
}

GlobalShaderData InitGlobalShaderData()
{
    ByteAddressBuffer globalParametersBuffer = ResourceDescriptorHeap[parameters.GlobalParametersBufferIndex];
    GlobalParameters globalParameters = globalParametersBuffer.Load<GlobalParameters>(0);

    ByteAddressBuffer meshInstanceBuffer = ResourceDescriptorHeap[globalParameters.GpuMeshInstanceBufferIndex];
    ByteAddressBuffer meshPrimitiveInstanceBuffer = ResourceDescriptorHeap[globalParameters.GpuMeshPrimitiveInstanceBufferIndex];
    ByteAddressBuffer materialBuffer = ResourceDescriptorHeap[globalParameters.MaterialBufferIndex];
    RaytracingAccelerationStructure raytracingAccelerationStructure = ResourceDescriptorHeap[parameters.AccelerationStructureIndex];

    GlobalShaderData globalShaderData;
    globalShaderData.MeshInstanceBuffer = meshInstanceBuffer;
    globalShaderData.MeshPrimitiveInstanceBuffer = meshPrimitiveInstanceBuffer;
    globalShaderData.MaterialBuffer = materialBuffer;
    globalShaderData.RaytracingAccelerationStructure = raytracingAccelerationStructure;

    globalShaderData.ViewProjMatrix = globalParameters.ViewProjMatrix;
    globalShaderData.InverseViewMatrix = globalParameters.InverseViewMatrix;
    globalShaderData.InverseProjectionMatrix = globalParameters.InverseProjectionMatrix;
    globalShaderData.Action = globalParameters.Action;

    return globalShaderData;
}

// TODO: Do other functions if we need less indirection
GpuDrawParameters GetDrawParametersNonUniform(GlobalShaderData globalShaderData, int32_t meshInstanceId, int32_t meshPrimitiveId)
{
    GpuMeshInstance meshInstance = globalShaderData.MeshInstanceBuffer.Load<GpuMeshInstance>(meshInstanceId * sizeof(GpuMeshInstance));

    ByteAddressBuffer meshBuffer = ResourceDescriptorHeap[NonUniformResourceIndex(meshInstance.MeshBufferIndex)];
    GpuMeshPrimitive meshPrimitive = meshBuffer.Load<GpuMeshPrimitive>(meshPrimitiveId * sizeof(GpuMeshPrimitive));

    ShaderMaterial material = globalShaderData.MaterialBuffer.Load<ShaderMaterial>(meshPrimitive.MaterialId * sizeof(ShaderMaterial));

    GpuDrawParameters result;
    result.MeshBuffer = meshBuffer;
    result.Material = material;
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

//! Constructs a special orthogonal matrix where the given normalized normal
//! vector is the third column
float3x3 get_shading_space(float3 n) {
	// This implementation is from: https://www.jcgt.org/published/0006/01/01/
	float s = (n.z > 0.0) ? 1.0 : -1.0;
	float a = -1.0 / (s + n.z);
	float b = n.x * n.y * a;
	float3 b1 = float3(1.0 + s * n.x * n.x * a, s * b, -s * n.x);
	float3 b2 = float3(b, s + n.y * n.y * a, -n.y);
	return float3x3(b1, b2, n);
}


// TODO: Optimize layout
struct RayHitInfo
{
    float3 WorldPosition;
    float3 WorldNormal;
    ShaderMaterial Material;
    bool HasHit;
};

RayHitInfo TraceRay(GlobalShaderData globalShaderData, float3 origin, float3 direction)
{
    RayHitInfo result;
    result.HasHit = false;

    RayQuery<RAY_FLAG_NONE> rayQuery;

    RayDesc ray;
	ray.Origin = origin;
	ray.TMin = 0.00001;
	ray.TMax = 10000.0;
	ray.Direction = direction;

    rayQuery.TraceRayInline(globalShaderData.RaytracingAccelerationStructure, RAY_FLAG_NONE, 0xFF, ray);

    while (rayQuery.Proceed());

    if (rayQuery.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
    {
        uint instanceId = rayQuery.CommittedInstanceID();
        uint geometryIndex = rayQuery.CommittedGeometryIndex();
        uint primitiveIndex = rayQuery.CommittedPrimitiveIndex();
        float2 bary = rayQuery.CommittedTriangleBarycentrics();

        GpuDrawParameters drawParameters = GetDrawParametersNonUniform(globalShaderData, instanceId, geometryIndex);
        
        uint3 indices = drawParameters.MeshBuffer.Load<uint3>(drawParameters.IndexBufferOffset + primitiveIndex * sizeof(uint3));

        MeshVertex vertexA = drawParameters.MeshBuffer.Load<MeshVertex>(drawParameters.VertexBufferOffset + indices.x * sizeof(MeshVertex));
        MeshVertex vertexB = drawParameters.MeshBuffer.Load<MeshVertex>(drawParameters.VertexBufferOffset + indices.y * sizeof(MeshVertex));
        MeshVertex vertexC = drawParameters.MeshBuffer.Load<MeshVertex>(drawParameters.VertexBufferOffset + indices.z * sizeof(MeshVertex));

        float3 hitPosition = vertexA.Position * (1 - bary.x - bary.y) + vertexB.Position * bary.x + vertexC.Position * bary.y;
        float3 hitNormal = normalize(vertexA.Normal * (1 - bary.x - bary.y) + vertexB.Normal * bary.x + vertexC.Normal * bary.y);

        result.HasHit = true;
        result.WorldPosition = RotateQuaternion(hitPosition, drawParameters.Rotation) * drawParameters.Scale + drawParameters.Translation;
        result.WorldNormal = RotateQuaternion(hitNormal, drawParameters.Rotation);
        result.Material = drawParameters.Material;
    }

    return result;
}


float3 sample_hemisphere_spherical(float2 randoms) 
{
    // Remap the random ranges
	float azimuth = (2.0 * M_PI) * randoms.x - M_PI;
	float inclination = (0.5 * M_PI) * randoms.y;

	float radius = sin(inclination);

	return float3(radius * cos(azimuth), radius * sin(azimuth), cos(inclination));
}

// TODO: SampledDirZ is cos angle
float get_hemisphere_spherical_density(float sampled_dir_z) 
{
	if (sampled_dir_z < 0.0)
		return 0.0;

    // sin²θ + cos²θ = 1
    float sinAngle = sqrt(max(0.0, 1.0 - sampled_dir_z * sampled_dir_z));

	return 1.0 / ((M_PI * M_PI) * sinAngle);
}

//! Produces a sample distributed uniformly with respect to projected solid
//! angle (PSA) in the upper hemisphere (positive z).
float3 sample_hemisphere_psa(float2 randoms) {
	// Sample a disk uniformly
	float azimuth = (2.0 * M_PI) * randoms[0] - M_PI;
	float radius = sqrt(randoms[1]);
	// Project to the hemisphere
	float z = sqrt(1.0 - radius * radius);
	return float3(radius * cos(azimuth), radius * sin(azimuth), z);
}


//! Returns the density w.r.t. solid angle sampled by sample_hemisphere_psa().
//! It only needs the z-coordinate of the sampled direction (in shading space).
float get_hemisphere_psa_density(float sampled_dir_z) {
	return (1.0 / M_PI) * max(0.0, sampled_dir_z);
}

float3 EvaluateSimpleBrdf(RayHitInfo hitInfo, float3 inDirection)
{
    return hitInfo.Material.AlbedoFactor / M_PI;
}

[shader("pixel")]
float4 PixelMain(const VertexOutput input) : SV_Target0
{
    GlobalShaderData globalShaderData = InitGlobalShaderData();
    uint32_t randomSeed = initRand((input.TextureCoordinates.x * 10000) * parameters.FrameIndex, (input.TextureCoordinates.y * 15000) * parameters.FrameIndex, 16);

    float2 textureCoordinatesJitter = float2(nextRand(randomSeed), nextRand(randomSeed)) * 0.001;

    float4 targetClipSpace = float4((input.TextureCoordinates.xy + textureCoordinatesJitter) * 2.0 - 1.0, 1.0, 1.0);
    float4 targetViewSpace = mul(targetClipSpace, globalShaderData.InverseProjectionMatrix);
    targetViewSpace.xyz /= targetViewSpace.w;

    float3 rayOriginViewSpace = float3(0.0, 0.0, 0.0);
    float3 rayDirectionViewSpace = normalize(targetViewSpace);

    float3 rayOriginWorldSpace = mul(float4(rayOriginViewSpace, 1), globalShaderData.InverseViewMatrix).xyz;
    float3 rayDirectionWorldSpace = mul(float4(rayDirectionViewSpace, 0), globalShaderData.InverseViewMatrix).xyz;
    rayDirectionWorldSpace = normalize(rayDirectionWorldSpace);

    uint32_t hitCount = 0;

    float3 pathTraceRayOrigin = rayOriginWorldSpace;
    float3 pathTraceRayDirection = rayDirectionWorldSpace;

    float3 radiance = float3(0.0, 0.0, 0.0);
    float3 weight = float3(1.0, 1.0, 1.0);

    for (uint32_t j = 0; j < parameters.PathTraceLength; j++)
    {
        RayHitInfo hitInfo = TraceRay(globalShaderData, pathTraceRayOrigin, pathTraceRayDirection);

        if (hitInfo.HasHit)
        {
            hitCount++;

            float3x3 shading_to_world_space = get_shading_space(hitInfo.WorldNormal);
            //float3 sampledDirection = sample_hemisphere_spherical(float2(nextRand(randomSeed), nextRand(randomSeed)));
            float3 sampledDirection = sample_hemisphere_psa(float2(nextRand(randomSeed), nextRand(randomSeed)));

            pathTraceRayOrigin = hitInfo.WorldPosition;
            pathTraceRayDirection = mul(sampledDirection, shading_to_world_space);

            float3 brdf = EvaluateSimpleBrdf(hitInfo, pathTraceRayDirection);
            // Sampled Direction Z in this case is already cos(O)
            float nDotL = sampledDirection.z;//max(dot(hitInfo.WorldNormal, pathTraceRayDirection), 0.0);
            //float density = get_hemisphere_spherical_density(sampledDirection.z);
            float density = get_hemisphere_psa_density(sampledDirection.z);

            radiance += hitInfo.Material.EmissiveFactor * weight;

            // TODO: Why multiplication with the previous weight here
            // TODO: Density is PDF
            weight *= brdf * nDotL / density;
        }
        else
        {
            //radiance += weight * float3(0.25, 0.5, 1.0) * 10;
            break;
        }
    }

    if (hitCount > 0)
    {
        return float4(radiance, 1.0);
    }

    return float4(0, 0, 0, 0);
}
