
// TODO: Shader parameters here are temporary
struct ShaderParameters
{
    uint32_t FrameDataBufferIndex;
    uint32_t MeshBuffer;
    uint32_t VertexBufferOffset;
    uint32_t MeshletOffset;
    uint32_t MeshletVertexIndexOffset;
    uint32_t MeshletTriangleIndexOffset;
    float3 Translation;
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
// TODO: Put that in an include file
float4x4 TransformMatrix(float4 quaternion, float3 translation)
{
	float xw = quaternion.x*quaternion.w, xx = quaternion.x*quaternion.x, yy = quaternion.y*quaternion.y,
      	yw = quaternion.y*quaternion.w, xy = quaternion.x*quaternion.y, yz = quaternion.y*quaternion.z,
      	zw = quaternion.z*quaternion.w, xz = quaternion.x*quaternion.z, zz = quaternion.z*quaternion.z;

	float4 row1 = float4(1-2*(yy+zz),  2*(xy+zw),  2*(xz-yw), 0.0);
	float4 row2 = float4(  2*(xy-zw),1-2*(xx+zz),  2*(yz+xw), 0.0);
	float4 row3 = float4(  2*(xz+yw),  2*(yz-xw),1-2*(xx+yy), 0.0);
	float4 row4 = float4(translation.x, translation.y, translation.z, 1.0);

    return float4x4(row1, row2, row3, row4);
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

        float4x4 worldMatrix = TransformMatrix(float4(0, 0, 0, 1), parameters.Translation);

        float4x4 inverseTransposeWorldMatrix = worldMatrix;

        uint vertexIndex = meshBuffer.Load<uint>(parameters.MeshletVertexIndexOffset + (meshlet.VertexIndexOffset + groupThreadId) * sizeof(uint));
        Vertex vertex = meshBuffer.Load<Vertex>(parameters.VertexBufferOffset + vertexIndex * sizeof(Vertex));

        //vertices[groupThreadId].Position = mul(float4(vertex.Position, 1.0), mul(worldMatrix, frameData.ViewProjMatrix));
        // NOTE: This calculation is faster because v * M is faster than M * M
        vertices[groupThreadId].Position = mul(mul(float4(vertex.Position, 1.0), worldMatrix), frameData.ViewProjMatrix);
        vertices[groupThreadId].WorldNormal = mul(vertex.Normal, inverseTransposeWorldMatrix); // TODO: Compute inverse transpose
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
