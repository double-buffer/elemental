struct ShaderParameters
{
    uint32_t MeshBuffer;
    uint32_t VertexBufferOffset;
    uint32_t MeshletOffset;
    uint32_t MeshletVertexIndexOffset;
    uint32_t MeshletTriangleIndexOffset;
    uint32_t Reserved1;
    uint32_t Reserved2;
    uint32_t Reserved3;
    float4 RotationQuaternion;
    float Zoom;
    float AspectRatio;
    uint32_t ShowMeshlets;
    uint32_t MeshletCount;
};

[[vk::push_constant]]
ShaderParameters parameters : register(b0);

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
    float3 WorldNormal: NORMAL0;
    uint   MeshletIndex : COLOR0;
};

// TODO: Put that in an include file
float4x4 TransformMatrix(float4 quaternion, float3 translation)
{
	float xw = quaternion.x*quaternion.w, xx = quaternion.x*quaternion.x, yy = quaternion.y*quaternion.y,
      	yw = quaternion.y*quaternion.w, xy = quaternion.x*quaternion.y, yz = quaternion.y*quaternion.z,
      	zw = quaternion.z*quaternion.w, xz = quaternion.x*quaternion.z, zz = quaternion.z*quaternion.z;

	float4 row1 = float4(1-2*(yy+zz),  2*(xy+zw),  2*(xz-yw), 0.0);
	float4 row2 = float4(  2*(xy-zw),1-2*(xx+zz),  2*(yz+xw), 0.0);
	float4 row3 = float4(  2*(xz+yw),  2*(yz-xw),1-2*(xx+yy), 0.0);
	float4 row4 = float4(0.0, 0.0, 0.0, 1.0);

    return float4x4(row1, row2, row3, row4);
}

float4x4 LookAtLHMatrix(float3 eyePosition, float3 targetPosition, float3 upDirection)
{
    float3 forwardDirection = normalize(targetPosition - eyePosition);
    float3 rightDirection = normalize(cross(upDirection, forwardDirection));
    float3 upDirectionNew = cross(forwardDirection, rightDirection);

    float4 row1 = float4(rightDirection.x, upDirectionNew.x, forwardDirection.x, 0.0);
    float4 row2 = float4(rightDirection.y, upDirectionNew.y, forwardDirection.y, 0.0);
    float4 row3 = float4(rightDirection.z, upDirectionNew.z, forwardDirection.z, 0.0);
    float4 row4 = float4(-dot(rightDirection, eyePosition), -dot(upDirectionNew, eyePosition), -dot(forwardDirection, eyePosition), 1.0);

    return float4x4(row1, row2, row3, row4);
}

float4x4 PerspectiveProjectionMatrix(float fovY, float aspectRatio, float zNear)
{
    float height = 1.0 / tan(fovY * 0.5);

    float4 row1 = float4(height / aspectRatio, 0.0f, 0.0f, 0.0f);
    float4 row2 = float4(0.0f, height, 0.0f, 0.0f);
    float4 row3 = float4(0.0f, 0.0f, 0, 1.0f);
    float4 row4 = float4(0.0f, 0.0f, zNear, 0.0f);

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
        // TODO: Compute matrix in task shader
        float cameraZDistance = (parameters.AspectRatio >= 0.75 ? -2.0 : -4.0) + parameters.Zoom;

        float4x4 worldMatrix = TransformMatrix(parameters.RotationQuaternion, float3(0.0, 0.0, 0.0));
        float4x4 viewMatrix = LookAtLHMatrix(float3(0, 0, cameraZDistance), float3(0, 0, 0), float3(0, 1, 0));
        float4x4 projectionMatrix = PerspectiveProjectionMatrix(0.78, parameters.AspectRatio, 0.001);

        float4x4 worldViewProjectionMatrix = mul(worldMatrix, mul(viewMatrix, projectionMatrix));
        float4x4 inverseTransposeWorldMatrix = worldMatrix;

        uint vertexIndex = meshBuffer.Load<uint>(parameters.MeshletVertexIndexOffset + (meshlet.VertexIndexOffset + groupThreadId) * sizeof(uint));
        Vertex vertex = meshBuffer.Load<Vertex>(parameters.VertexBufferOffset + vertexIndex * sizeof(Vertex));

        vertices[groupThreadId].Position = mul(float4(vertex.Position, 1.0), worldViewProjectionMatrix);
        vertices[groupThreadId].WorldNormal = mul(float4(vertex.Normal, 0.0), inverseTransposeWorldMatrix).xyz; // TODO: Compute inverse transpose
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
    if (parameters.ShowMeshlets == 0)
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
