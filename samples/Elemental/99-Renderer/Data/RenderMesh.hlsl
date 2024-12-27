
// TODO: Shader parameters here are temporary
struct ShaderParameters
{
    uint32_t FrameDataBufferIndex;
    uint32_t MeshBuffer;
    uint32_t VertexBufferOffset;
    uint32_t MeshletOffset;
    uint32_t MeshletVertexIndexOffset;
    uint32_t MeshletTriangleIndexOffset;
    uint32_t Reserved1;
    float Scale;
    float3 Translation;
    uint32_t Reserved2;
    float4 Rotation;
};

[[vk::push_constant]]
ShaderParameters parameters : register(b0);

struct FrameData
{
    float4x4 ViewProjMatrix;
    uint32_t ShowMeshlets;
};

struct ElemMeshlet
{
    uint32_t VertexIndexOffset;
    uint32_t VertexIndexCount;
    uint32_t TriangleOffset;
    uint32_t TriangleCount;
};

struct Vertex
{
    float3 Position;
    float3 Normal;
    float2 TextureCoordinates;
};

struct VertexOutput
{
    float4 Position: SV_Position;
    float3 WorldNormal: NORMAL0;
    uint   MeshletIndex : COLOR0;
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

        //vertices[groupThreadId].Position = mul(float4(vertex.Position, 1.0), mul(worldMatrix, frameData.ViewProjMatrix));
        // NOTE: This calculation is faster because v * M is faster than M * M
        vertices[groupThreadId].Position = mul(float4(worldPosition, 1.0), frameData.ViewProjMatrix);
        vertices[groupThreadId].WorldNormal = worldNormal;
        vertices[groupThreadId].MeshletIndex = groupId;
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
    // TODO: Get the framedata only in the mesh shader?
    ByteAddressBuffer frameDataBuffer = ResourceDescriptorHeap[parameters.FrameDataBufferIndex];
    FrameData frameData = frameDataBuffer.Load<FrameData>(0);

    if (frameData.ShowMeshlets == 0)
    {
        return float4(normalize(input.WorldNormal) * 0.5 + 0.5, 1.0);
    }
    else
    {
        uint hashResult = hash(input.MeshletIndex);
        float3 meshletColor = float3(float(hashResult & 255), float((hashResult >> 8) & 255), float((hashResult >> 16) & 255)) / 255.0;

        return float4(meshletColor, 1.0);
    }
}
