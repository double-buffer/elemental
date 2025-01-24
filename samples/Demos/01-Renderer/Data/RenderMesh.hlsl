
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
    uint32_t TextureSampler;
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

struct ElemMeshlet
{
    uint32_t VertexIndexOffset;
    uint32_t VertexIndexCount;
    uint32_t TriangleOffset;
    uint32_t TriangleCount;
};

// Compress Data
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
    float4 WorldTangent: Attribute5;
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
        vertices[groupThreadId].WorldTangent = float4(worldTangent, vertex.Tangent.w);
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
    // TODO: Get the framedata only in the mesh shader?
     ByteAddressBuffer globalParametersBuffer = ResourceDescriptorHeap[parameters.GlobalParametersBufferIndex];
    GlobalParameters globalParameters = globalParametersBuffer.Load<GlobalParameters>(0);

    ByteAddressBuffer materialBuffer = ResourceDescriptorHeap[globalParameters.MaterialBufferIndex];
    ShaderMaterial material = materialBuffer.Load<ShaderMaterial>(input.MaterialId * sizeof(ShaderMaterial));

    SamplerState textureSampler = SamplerDescriptorHeap[globalParameters.TextureSampler];

    if (globalParameters.Action == 0 && input.MaterialId >= 0)
    {
        float3 worldNormal = input.WorldNormal;
        float4 albedo = float4(1, 1, 1, 1);

        if (material.AlbedoTextureId >= 0)
        {
            // TODO: Should we use non uniform index here? We know we have the same material for each meshlet.
            // But we sometimes may group some meshlets together
            Texture2D<float4> albedoTexture = ResourceDescriptorHeap[NonUniformResourceIndex(material.AlbedoTextureId)];
            albedo = albedoTexture.Sample(textureSampler, input.TextureCoordinates) * material.AlbedoFactor;

            // TODO: Doing discard on the main pass is really bad for performance.
            // Doing it disable the early depth test in the shader, so all pixel shader code has to run for
            // occluded pixels.
            // TODO: We will need to process transparent objects in another path in another shader
            if (albedo.a < 0.5)
            {
                discard;
            }

        //    return albedo;
        }

        if (material.NormalTextureId >= 0)
        {
            Texture2D<float4> normalTexture = ResourceDescriptorHeap[NonUniformResourceIndex(material.NormalTextureId)];
            float3 normalMap = normalTexture.Sample(textureSampler, input.TextureCoordinates).rgb * 2.0 - 1.0;
            //return float4(normalMap, 1);

            float3 bitangent = cross(worldNormal, input.WorldTangent.xyz) * input.WorldTangent.w;
	        worldNormal = normalize(normalMap.x * input.WorldTangent.xyz + normalMap.y * bitangent + normalMap.z * worldNormal);

            //float3 surfaceGradient = float3(TspaceNormalToDerivative(normalMap), 0);
            //worldNormal = ResolveNormalFromSurfaceGradient(worldNormal, surfaceGradient);
        }

        float nDotL = max(dot(worldNormal, normalize(float3(-1.0, 1.0, 1.0))), 0.0);
        float ambient = 0.05;

        return albedo * (nDotL + ambient);
        return float4(worldNormal * 0.5 + 0.5, 1.0);

        if (input.WorldTangent.w > 0)
        {
            return float4(1, 0, 0, 1);
        }
        else
        {
            return float4(0, 1, 0, 1);
        }

        //return float4(0, 0, 1, 1);;
        //return float4(input.Tangent * 0.5 + 0.5, 1.0);
    }
    else
    {
        //uint hashResult = hash(input.MeshletIndex);
        uint hashResult = hash(input.MaterialId);
        float3 meshletColor = float3(float(hashResult & 255), float((hashResult >> 8) & 255), float((hashResult >> 16) & 255)) / 255.0;

        return float4(input.WorldNormal * 0.5 + 0.5, 1.0);
        return float4(meshletColor, 1.0);
    }
}
