struct ShaderParameters
{
    uint32_t GlobalParametersBufferIndex;
    uint32_t MeshPrimitiveInstanceId;
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
    uint32_t MeshletOffset;
    float4 Rotation;
    float3 Translation;
    float Scale;
};

// TODO: Do other functions if we need less indirection
GpuDrawParameters GetDrawParameters(GlobalParameters globalParameters, int32_t meshPrimitiveInstanceId)
{
    ByteAddressBuffer meshInstanceBuffer = ResourceDescriptorHeap[globalParameters.GpuMeshInstanceBufferIndex];
    ByteAddressBuffer meshPrimitiveInstanceBuffer = ResourceDescriptorHeap[globalParameters.GpuMeshPrimitiveInstanceBufferIndex];

    GpuMeshPrimitiveInstance meshPrimitiveInstance = meshPrimitiveInstanceBuffer.Load<GpuMeshPrimitiveInstance>(meshPrimitiveInstanceId * sizeof(GpuMeshPrimitiveInstance));

    GpuMeshInstance meshInstance = meshInstanceBuffer.Load<GpuMeshInstance>(meshPrimitiveInstance.MeshInstanceId * sizeof(GpuMeshInstance));
    // TODO: Be carreful with uniform/nonuniform (normally it should be uniform here because we process one primitive per meshlet but for raytracing sometimes it isn't)
    ByteAddressBuffer meshBuffer = ResourceDescriptorHeap[meshInstance.MeshBufferIndex];

    GpuMeshPrimitive meshPrimitive = meshBuffer.Load<GpuMeshPrimitive>(meshPrimitiveInstance.MeshPrimitiveId * sizeof(GpuMeshPrimitive));

    GpuDrawParameters result;
    result.MeshBuffer = meshBuffer;
    result.MaterialId = meshPrimitive.MaterialId;
    result.VertexBufferOffset = meshPrimitive.VertexBufferOffset;
    result.MeshletOffset = meshPrimitive.MeshletOffset;
    result.Rotation = meshInstance.Rotation;
    result.Translation = meshInstance.Translation;
    result.Scale = meshInstance.Scale;

    return result;
}

typedef struct
{
    int32_t AlbedoTextureId;
    int32_t NormalTextureId;
    float4 AlbedoFactor;
    float3 EmissiveFactor;
} ShaderMaterial;

// TODO: Compress
struct ElemMeshlet
{
    uint32_t VertexIndexOffset;
    uint32_t VertexIndexCount;
    uint32_t TriangleOffset;
    uint32_t TriangleCount;
};

// NOTE: The vertex layout is not optimized to not add complexity to the sample. 
// Do not use this on production code.
struct Vertex
{
    float3 Position;
    float3 Normal;
    float4 Tangent;
    float2 TextureCoordinates;
};

struct VertexOutput
{
    float4 Position: SV_Position;
    float3 WorldNormal: Attribute0;
    float2 TextureCoordinates: Attribute1;
    nointerpolation uint MeshletIndex: Attribute2;
    nointerpolation uint MaterialId: Attribute3;
};

#define IDENTITY_MATRIX float4x4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1)

float3 RotateQuaternion(float3 v, float4 q)
{
	return v + 2.0 * cross(q.xyz, cross(q.xyz, v) + q.w * v);
}

[shader("mesh")]
[OutputTopology("triangle")]
[NumThreads(126, 1, 1)]
void MeshMain(in uint groupId: SV_GroupID, 
              in uint groupThreadId : SV_GroupThreadID, 
              out vertices VertexOutput vertices[64], 
              out indices uint3 indices[126])
{
    uint meshletIndex = groupId;

    ByteAddressBuffer globalParametersBuffer = ResourceDescriptorHeap[parameters.GlobalParametersBufferIndex];
    GlobalParameters globalParameters = globalParametersBuffer.Load<GlobalParameters>(0);

    GpuDrawParameters drawParameters = GetDrawParameters(globalParameters, parameters.MeshPrimitiveInstanceId);

    ElemMeshlet meshlet = drawParameters.MeshBuffer.Load<ElemMeshlet>(drawParameters.MeshletOffset + meshletIndex * sizeof(ElemMeshlet));

    SetMeshOutputCounts(meshlet.VertexIndexCount, meshlet.TriangleCount);

    if (groupThreadId < meshlet.VertexIndexCount)
    {
        uint vertexIndex = drawParameters.MeshBuffer.Load<uint>(meshlet.VertexIndexOffset + groupThreadId * sizeof(uint));
        Vertex vertex = drawParameters.MeshBuffer.Load<Vertex>(drawParameters.VertexBufferOffset + vertexIndex * sizeof(Vertex));

        float3 worldPosition = RotateQuaternion(vertex.Position, drawParameters.Rotation) * drawParameters.Scale + drawParameters.Translation;
        float3 worldNormal = RotateQuaternion(vertex.Normal, drawParameters.Rotation);
        float3 worldTangent = RotateQuaternion(vertex.Tangent.xyz, drawParameters.Rotation);

        vertices[groupThreadId].Position = mul(float4(worldPosition, 1.0), globalParameters.ViewProjMatrix);
        vertices[groupThreadId].WorldNormal = worldNormal;
        vertices[groupThreadId].TextureCoordinates = vertex.TextureCoordinates;
        vertices[groupThreadId].MeshletIndex = groupId;
        vertices[groupThreadId].MaterialId = drawParameters.MaterialId;
    }

    if (groupThreadId < meshlet.TriangleCount)
    {
        uint triangleIndex = drawParameters.MeshBuffer.Load<uint>(meshlet.TriangleOffset + groupThreadId * sizeof(uint));
        indices[groupThreadId] = unpack_u8u32(triangleIndex).xyz;
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
    ByteAddressBuffer globalParametersBuffer = ResourceDescriptorHeap[parameters.GlobalParametersBufferIndex];
    GlobalParameters globalParameters = globalParametersBuffer.Load<GlobalParameters>(0);

    ByteAddressBuffer materialBuffer = ResourceDescriptorHeap[globalParameters.MaterialBufferIndex];
    ShaderMaterial material = materialBuffer.Load<ShaderMaterial>(input.MaterialId * sizeof(ShaderMaterial));

    float4 albedo = material.AlbedoFactor;
    float3 worldNormal = normalize(input.WorldNormal);
    float nDotL = max(dot(worldNormal, normalize(float3(1.0, 1.0, -1.0))), 0.0);
    float ambient = 0.05;

    return float4(albedo.rgb * (nDotL + ambient), 1.0);
}
