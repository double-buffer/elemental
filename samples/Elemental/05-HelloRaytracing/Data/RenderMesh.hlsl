struct ShaderParameters
{
    uint32_t FrameDataBufferIndex;
    uint32_t MeshBuffer;
    uint32_t MaterialBuffer;
    uint32_t VertexBufferOffset;
    uint32_t MeshletOffset;
    uint32_t MeshletVertexIndexOffset;
    uint32_t MeshletTriangleIndexOffset;
    float Scale;
    float3 Translation;
    uint32_t Reserved;
    float4 Rotation;
    uint32_t MaterialId;
    uint32_t TextureSampler;
};


[[vk::push_constant]]
ShaderParameters parameters : register(b0);

struct FrameData
{
    float4x4 ViewProjMatrix;
    uint32_t ShowMeshlets;
};

typedef struct
{
    int32_t AlbedoTextureId;
    int32_t NormalTextureId;
    float4 AlbedoFactor;
} ShaderMaterial;

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

    ByteAddressBuffer meshBuffer = ResourceDescriptorHeap[parameters.MeshBuffer];
    ElemMeshlet meshlet = meshBuffer.Load<ElemMeshlet>(parameters.MeshletOffset + meshletIndex * sizeof(ElemMeshlet));

    SetMeshOutputCounts(meshlet.VertexIndexCount, meshlet.TriangleCount);

    if (groupThreadId < meshlet.VertexIndexCount)
    {
        ByteAddressBuffer frameDataBuffer = ResourceDescriptorHeap[parameters.FrameDataBufferIndex];
        FrameData frameData = frameDataBuffer.Load<FrameData>(0);

        uint vertexIndex = meshBuffer.Load<uint>(parameters.MeshletVertexIndexOffset + (meshlet.VertexIndexOffset + groupThreadId) * sizeof(uint));
        Vertex vertex = meshBuffer.Load<Vertex>(parameters.VertexBufferOffset + vertexIndex * sizeof(Vertex));

        float3 worldPosition = RotateQuaternion(vertex.Position, parameters.Rotation) * parameters.Scale + parameters.Translation;
        float3 worldNormal = RotateQuaternion(vertex.Normal, parameters.Rotation);
        float3 worldTangent = RotateQuaternion(vertex.Tangent.xyz, parameters.Rotation);

        //vertices[groupThreadId].Position = mul(float4(vertex.Position, 1.0), mul(worldMatrix, frameData.ViewProjMatrix));
        // NOTE: This calculation is faster because v * M is faster than M * M
        vertices[groupThreadId].Position = mul(float4(worldPosition, 1.0), frameData.ViewProjMatrix);
        vertices[groupThreadId].WorldNormal = worldNormal;
        vertices[groupThreadId].TextureCoordinates = vertex.TextureCoordinates;
        vertices[groupThreadId].MeshletIndex = groupId;
        vertices[groupThreadId].MaterialId = parameters.MaterialId;
    }

    if (groupThreadId < meshlet.TriangleCount)
    {
        uint triangleIndex = meshBuffer.Load<uint>(parameters.MeshletTriangleIndexOffset + (meshlet.TriangleOffset + groupThreadId) * sizeof(uint));
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
    ByteAddressBuffer frameDataBuffer = ResourceDescriptorHeap[parameters.FrameDataBufferIndex];
    FrameData frameData = frameDataBuffer.Load<FrameData>(0);

    ByteAddressBuffer materialBuffer = ResourceDescriptorHeap[parameters.MaterialBuffer];
    ShaderMaterial material = materialBuffer.Load<ShaderMaterial>(parameters.MaterialId * sizeof(ShaderMaterial));

    if (frameData.ShowMeshlets == 0)
    {
        float4 albedo = material.AlbedoFactor;
        float3 worldNormal = normalize(input.WorldNormal);
        float nDotL = max(dot(worldNormal, normalize(float3(1.0, 1.0, -1.0))), 0.0);
        float ambient = 0.05;

        return albedo * (nDotL + ambient);
    }
    else
    {
        uint hashResult = hash(input.MaterialId);
        float3 meshletColor = float3(float(hashResult & 255), float((hashResult >> 8) & 255), float((hashResult >> 16) & 255)) / 255.0;

        return float4(meshletColor, 1.0);
    }
}
