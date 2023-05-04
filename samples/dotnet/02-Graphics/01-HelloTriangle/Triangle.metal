using namespace metal;

#include <metal_stdlib>
#include <simd/simd.h>

struct VertexOutput
{
    float4 Position [[position]];
    float4 Color [[user(locn0)]];
};

struct PrimitiveOutput
{
};

struct ShaderParameters
{
    float RotationY;
};

constant float3 triangleVertices[] =
{
    float3(-0.5, 0.5, 0),
    float3(0.5, 0.5, 0),
    float3(-0.5, -0.5, 0)
};

constant float4 triangleColors[] =
{
    float4(1.0, 0.0, 0, 1),
    float4(0.0, 1.0, 0, 1),
    float4(0.0, 0.0, 1, 1)
};

constant uint8_t rectangleIndices[] =
{
    0, 1, 2
};

float3 rotate(float3 position, float pitch, float roll, float yaw) 
{
    float cosa = cos(yaw);
    float sina = sin(yaw);

    float cosb = cos(pitch);
    float sinb = sin(pitch);

    float cosc = cos(roll);
    float sinc = sin(roll);

    float Axx = cosa*cosb;
    float Axy = cosa*sinb*sinc - sina*cosc;
    float Axz = cosa*sinb*cosc + sina*sinc;

    float Ayx = sina*cosb;
    float Ayy = sina*sinb*sinc + cosa*cosc;
    float Ayz = sina*sinb*cosc - cosa*sinc;

    float Azx = -sinb;
    float Azy = cosb*sinc;
    float Azz = cosb*cosc;

    float px = position.x;
    float py = position.y;
    float pz = position.z;

    float3 result;

    result.x = Axx*px + Axy*py + Axz*pz;
    result.y = Ayx*px + Ayy*py + Ayz*pz;
    result.z = Azx*px + Azy*py + Azz*pz;

    return result;
}

constant uint32_t meshVertexCount = 3;
constant uint32_t meshPrimitiveCount = 1;

using AAPLTriangleMeshType = metal::mesh<VertexOutput, PrimitiveOutput, meshVertexCount, meshPrimitiveCount, metal::topology::triangle>;

/// The mesh stage function that generates a triangle mesh.
[[mesh, max_total_threads_per_threadgroup(126)]]
void MeshMain(AAPLTriangleMeshType output,
                                 uint32_t groupThreadId [[thread_index_in_threadgroup]],
                                 uint32_t groupId [[threadgroup_position_in_grid]],
                                 device const ShaderParameters &parameters [[buffer(0)]])
{


    output.set_primitive_count(meshPrimitiveCount);
  
    if (groupThreadId < meshVertexCount)
    {
        VertexOutput vertexOutput = {};
        
        float3 position = triangleVertices[groupThreadId];
        
        position = rotate(position, parameters.RotationY, 0, 0);
        position.z = 0.5;

        vertexOutput.Position = float4(position, 1.0);
        vertexOutput.Color = triangleColors[groupThreadId];
        
        output.set_vertex(groupThreadId, vertexOutput);
    }

    if (groupThreadId < meshPrimitiveCount)
    {
        PrimitiveOutput primitiveOutput;
        output.set_primitive(groupThreadId, primitiveOutput);
        
        uint i = (3*groupThreadId);
        output.set_index(i+0, rectangleIndices[i+0]);
        output.set_index(i+1, rectangleIndices[i+1]);
        output.set_index(i+2, rectangleIndices[i+2]);
    }  
}