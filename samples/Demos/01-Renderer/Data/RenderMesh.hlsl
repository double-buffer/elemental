#include "ShaderData.h"

[[vk::push_constant]]
ShaderParameters parameters : register(b0);

struct ElemMeshlet
{
    uint32_t VertexIndexOffset;
    uint32_t VertexIndexCount;
    uint32_t TriangleOffset;
    uint32_t TriangleCount;
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

struct GlobalShaderData
{
    ByteAddressBuffer MeshInstanceBuffer;
    ByteAddressBuffer MeshPrimitiveInstanceBuffer;
    ByteAddressBuffer MaterialBuffer;
    RaytracingAccelerationStructure RaytracingAccelerationStructure;
    SamplerState TextureSampler;
    float4x4 ViewProjMatrix;
    float4x4 InverseViewMatrix;
    float4x4 InverseProjectionMatrix;
    uint32_t Action;
};

GlobalShaderData InitGlobalShaderData()
{
    ByteAddressBuffer globalParametersBuffer = ResourceDescriptorHeap[parameters.GlobalParametersBufferIndex];
    ShaderGlobalParameters globalParameters = globalParametersBuffer.Load<ShaderGlobalParameters>(0);

    ByteAddressBuffer meshInstanceBuffer = ResourceDescriptorHeap[globalParameters.MeshInstanceBufferIndex];
    ByteAddressBuffer meshPrimitiveInstanceBuffer = ResourceDescriptorHeap[globalParameters.MeshPrimitiveInstanceBufferIndex];
    ByteAddressBuffer materialBuffer = ResourceDescriptorHeap[globalParameters.MaterialBufferIndex];
    RaytracingAccelerationStructure raytracingAccelerationStructure = ResourceDescriptorHeap[parameters.AccelerationStructureIndex];
    SamplerState textureSampler = SamplerDescriptorHeap[globalParameters.TextureSampler];

    GlobalShaderData globalShaderData;
    globalShaderData.MeshInstanceBuffer = meshInstanceBuffer;
    globalShaderData.MeshPrimitiveInstanceBuffer = meshPrimitiveInstanceBuffer;
    globalShaderData.MaterialBuffer = materialBuffer;
    globalShaderData.RaytracingAccelerationStructure = raytracingAccelerationStructure;
    globalShaderData.TextureSampler = textureSampler;

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

    ShaderMaterial material = (ShaderMaterial)0; 

    if (meshPrimitive.MaterialId >= 0)
    {
        material = globalShaderData.MaterialBuffer.Load<ShaderMaterial>(meshPrimitive.MaterialId * sizeof(ShaderMaterial));
    }

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


// TODO: Do other functions if we need less indirection
GpuDrawParameters GetDrawParameters(GlobalShaderData globalShaderData, int32_t meshPrimitiveInstanceId)
{
    GpuMeshPrimitiveInstance meshPrimitiveInstance = globalShaderData.MeshPrimitiveInstanceBuffer.Load<GpuMeshPrimitiveInstance>(meshPrimitiveInstanceId * sizeof(GpuMeshPrimitiveInstance));

    GpuMeshInstance meshInstance = globalShaderData.MeshInstanceBuffer.Load<GpuMeshInstance>(meshPrimitiveInstance.MeshInstanceId * sizeof(GpuMeshInstance));
    // TODO: Be carreful with uniform/nonuniform (normally it should be uniform here because we process one primitive per meshlet but for raytracing sometimes it isn't)
    ByteAddressBuffer meshBuffer = ResourceDescriptorHeap[meshInstance.MeshBufferIndex];

    GpuMeshPrimitive meshPrimitive = meshBuffer.Load<GpuMeshPrimitive>(meshPrimitiveInstance.MeshPrimitiveId * sizeof(GpuMeshPrimitive));

    ShaderMaterial material = (ShaderMaterial)0; 

    if (meshPrimitive.MaterialId >= 0)
    {
        material = globalShaderData.MaterialBuffer.Load<ShaderMaterial>(meshPrimitive.MaterialId * sizeof(ShaderMaterial));
    }

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

// Compress Data
struct Vertex
{
    float3 Position;
    float3 Normal;
    float4 Tangent;
    float2 TextureCoordinates;
};

// TODO: https://gpuopen.com/learn/mesh_shaders/mesh_shaders-optimization_and_best_practices/
// We can put the non interpolated attributes in primitive attributes
// TODO: Rename to VertexAttribute
struct VertexOutput
{
    float4 Position: SV_Position;
    float3 WorldPosition: Attribute6;
    float3 WorldNormal: Attribute0;
    float4 WorldTangent: Attribute5;
    float2 TextureCoordinates: Attribute1;
    nointerpolation uint MeshletIndex: Attribute2;
    nointerpolation uint MeshPrimitiveInstanceId: Attribute3;
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
    // TODO: Cull primitives and use SV_CullPrimitive
    GlobalShaderData globalShaderData = InitGlobalShaderData();
    uint meshletIndex = groupId;

    GpuDrawParameters drawParameters = GetDrawParameters(globalShaderData, parameters.MeshPrimitiveInstanceId);

    ElemMeshlet meshlet = drawParameters.MeshBuffer.Load<ElemMeshlet>(drawParameters.MeshletOffset + meshletIndex * sizeof(ElemMeshlet));

    SetMeshOutputCounts(meshlet.VertexIndexCount, meshlet.TriangleCount);

    if (groupThreadId < meshlet.VertexIndexCount)
    {
        uint vertexIndex = drawParameters.MeshBuffer.Load<uint>(meshlet.VertexIndexOffset + groupThreadId * sizeof(uint));
        Vertex vertex = drawParameters.MeshBuffer.Load<Vertex>(drawParameters.VertexBufferOffset + vertexIndex * sizeof(Vertex));

        float3 worldPosition = RotateQuaternion(vertex.Position, drawParameters.Rotation) * drawParameters.Scale + drawParameters.Translation;
        float3 worldNormal = RotateQuaternion(vertex.Normal, drawParameters.Rotation);
        float3 worldTangent = RotateQuaternion(vertex.Tangent.xyz, drawParameters.Rotation);

        vertices[groupThreadId].Position = mul(float4(worldPosition, 1.0), globalShaderData.ViewProjMatrix);
        vertices[groupThreadId].WorldPosition = worldPosition;
        vertices[groupThreadId].WorldNormal = worldNormal;
        vertices[groupThreadId].WorldTangent = float4(worldTangent, vertex.Tangent.w);
        vertices[groupThreadId].TextureCoordinates = vertex.TextureCoordinates;
        vertices[groupThreadId].MeshletIndex = groupId;
        vertices[groupThreadId].MeshPrimitiveInstanceId = parameters.MeshPrimitiveInstanceId;
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

float TraceShadowRay(GlobalShaderData globalShaderData, float3 origin, float3 direction)
{
    RayQuery<RAY_FLAG_NONE> rayQuery;

    RayDesc ray;
	ray.Origin = origin;
	ray.TMin = 0.01;
	ray.TMax = 10000.0;
	ray.Direction = direction;

    rayQuery.TraceRayInline(globalShaderData.RaytracingAccelerationStructure, RAY_FLAG_NONE, 0xFF, ray);

    while (rayQuery.Proceed())
    {
        uint instanceId = rayQuery.CandidateInstanceID();
        uint geometryIndex = rayQuery.CandidateGeometryIndex();
        uint primitiveIndex = rayQuery.CandidatePrimitiveIndex();
        float2 bary = rayQuery.CandidateTriangleBarycentrics();

        GpuDrawParameters drawParameters = GetDrawParametersNonUniform(globalShaderData, instanceId, geometryIndex);
        
        uint3 indices = drawParameters.MeshBuffer.Load<uint3>(drawParameters.IndexBufferOffset + primitiveIndex * sizeof(uint3));

        Vertex vertexA = drawParameters.MeshBuffer.Load<Vertex>(drawParameters.VertexBufferOffset + indices.x * sizeof(Vertex));
        Vertex vertexB = drawParameters.MeshBuffer.Load<Vertex>(drawParameters.VertexBufferOffset + indices.y * sizeof(Vertex));
        Vertex vertexC = drawParameters.MeshBuffer.Load<Vertex>(drawParameters.VertexBufferOffset + indices.z * sizeof(Vertex));

        float2 hitTextureCoordinates = vertexA.TextureCoordinates * (1 - bary.x - bary.y) + vertexB.TextureCoordinates * bary.x + vertexC.TextureCoordinates * bary.y;

        float alpha = 1.0;

        if (drawParameters.Material.TransparentMode == ShaderMaterialTransparentMode_Alpha && drawParameters.Material.AlbedoTextureId >= 0)
        {
            Texture2D<float4> albedoTexture = ResourceDescriptorHeap[NonUniformResourceIndex(drawParameters.Material.AlbedoTextureId)];
            alpha = albedoTexture.SampleLevel(globalShaderData.TextureSampler, hitTextureCoordinates, 0).a;
        }
            
        if (alpha >= drawParameters.Material.AlphaCutoff)
        {
            rayQuery.CommitNonOpaqueTriangleHit();
        }
    }

    if (rayQuery.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
    {
        return 0.0;
    }

    return 1.0;
}

[shader("pixel")]
float4 PixelMain(const VertexOutput input) : SV_Target0
{
    GlobalShaderData globalShaderData = InitGlobalShaderData();
    GpuDrawParameters drawParameters = GetDrawParameters(globalShaderData, input.MeshPrimitiveInstanceId);

    if (globalShaderData.Action == 0 && drawParameters.Material.IsLoaded)
    {
        float3 worldNormal = input.WorldNormal;
        float4 albedo = drawParameters.Material.AlbedoFactor;

        if (drawParameters.Material.AlbedoTextureId >= 0)
        {
            // TODO: Should we use non uniform index here? We know we have the same material for each meshlet.
            // But we sometimes may group some meshlets together
            Texture2D<float4> albedoTexture = ResourceDescriptorHeap[NonUniformResourceIndex(drawParameters.Material.AlbedoTextureId)];
            albedo *= albedoTexture.Sample(globalShaderData.TextureSampler, input.TextureCoordinates);

            // TODO: Doing discard on the main pass is really bad for performance.
            // Doing it disable the early depth test in the shader, so all pixel shader code has to run for
            // occluded pixels.
            // TODO: We will need to process transparent objects in another path in another shader

            if (drawParameters.Material.TransparentMode == ShaderMaterialTransparentMode_Alpha && albedo.a < drawParameters.Material.AlphaCutoff)
            {
                discard;
            }

        //    return albedo;
        }

        if (drawParameters.Material.NormalTextureId >= 0)
        {
            Texture2D<float4> normalTexture = ResourceDescriptorHeap[NonUniformResourceIndex(drawParameters.Material.NormalTextureId)];
            float3 normalMap = normalTexture.Sample(globalShaderData.TextureSampler, input.TextureCoordinates).rgb * 2.0 - 1.0;

            float3 bitangent = cross(worldNormal, input.WorldTangent.xyz) * input.WorldTangent.w;
	        worldNormal = normalize(normalMap.x * input.WorldTangent.xyz + normalMap.y * bitangent + normalMap.z * worldNormal);

            //return float4(worldNormal * 0.5 + 0.5, 1);
        }

        float3 lightDirection = normalize(float3(-0.2, 1.0, -0.4));

        float shadowContribution = TraceShadowRay(globalShaderData, input.WorldPosition, lightDirection);

        float nDotL = max(dot(worldNormal, lightDirection), 0.0) * float3(100, 90, 80) * 1.5;
        //float3 ambient = float3(0.25, 0.5, 1.0) * 10; //float3(0.5, 0.5, 0.5);
        float3 ambient = float3(0.5, 0.5, 0.7) * 10;

        //float3 outputColor = material.EmissiveFactor + albedo.rgb * (nDotL + ambient);
        float3 outputColor = albedo.rgb * (nDotL * shadowContribution + ambient);

        return float4(outputColor, 1.0);
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
        uint hashResult = hash(input.MeshPrimitiveInstanceId);
        float3 meshletColor = float3(float(hashResult & 255), float((hashResult >> 8) & 255), float((hashResult >> 16) & 255)) / 255.0;

        return float4(input.WorldNormal * 0.5 + 0.5, 1.0);
        return float4(meshletColor, 1.0);
    }
}
