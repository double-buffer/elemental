using namespace metal;

#include <metal_stdlib>
#include <simd/simd.h>

struct VertexOutput
{
    float4 Position [[position]];
    float4 Color;
};

struct PrimitiveOutput
{
};

constant float4 triangleVertices[] =
{
    float4(-0.5, 0.5, 0, 1),
    float4(0.5, 0.5, 0, 1),
    float4(-0.5, -0.5, 0, 1)
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

constant uint32_t meshVertexCount = 3;
constant uint32_t meshPrimitiveCount = 1;

using AAPLTriangleMeshType = metal::mesh<VertexOutput, PrimitiveOutput, meshVertexCount, meshPrimitiveCount, metal::topology::triangle>;

/// The mesh stage function that generates a triangle mesh.
[[mesh, max_total_threads_per_threadgroup(126)]]
void MeshMain(AAPLTriangleMeshType output,
                                 uint32_t groupThreadId [[thread_index_in_threadgroup]],
                                 uint32_t groupId [[threadgroup_position_in_grid]])
{


    output.set_primitive_count(meshPrimitiveCount);
  
    if (groupThreadId < meshVertexCount)
    {
        VertexOutput vertexOutput = {};
        vertexOutput.Position = triangleVertices[groupThreadId];
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


struct PixelOutput
{
    float4 Color [[color(0)]];
};

fragment PixelOutput PixelMain(VertexOutput input [[stage_in]])
{
    PixelOutput output {};

    output.Color = input.Color;
    
    return output; 
}