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
    float RotationX;
    float RotationY;
    float RotationZ;
    float TranslationX;
    float TranslationY;
    float TranslationZ;
    float AspectRatio;
    uint CurrentColorIndex;
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

float4x4 RotationMatrix(float rotationX, float rotationY, float rotationZ)
{
    float cosX = cos(rotationX);
    float sinX = sin(rotationX);
    float cosY = cos(rotationY);
    float sinY = sin(rotationY);
    float cosZ = cos(rotationZ);
    float sinZ = sin(rotationZ);

    float4 row1 = float4(cosY * cosZ, sinX * sinY * cosZ - cosX * sinZ, cosX * sinY * cosZ + sinX * sinZ, 0.0f);
    float4 row2 = float4(cosY * sinZ, sinX * sinY * sinZ + cosX * cosZ, cosX * sinY * sinZ - sinX * cosZ, 0.0f);
    float4 row3 = float4(-sinY, sinX * cosY, cosX * cosY, 0.0f);
    float4 row4 = float4(0.0f, 0.0f, 0.0f, 1.0f);

    return float4x4(row1, row2, row3, row4);
}

float4x4 LookAtLHMatrix(float3 eyePosition, float3 targetPosition, float3 upDirection)
{
    float3 forwardDirection = normalize(targetPosition - eyePosition);
    float3 rightDirection = normalize(cross(upDirection, forwardDirection));
    float3 upDirectionNew = cross(forwardDirection, rightDirection);

    float4 row1 = float4(rightDirection.x, upDirectionNew.x, forwardDirection.x, 0.0f);
    float4 row2 = float4(rightDirection.y, upDirectionNew.y, forwardDirection.y, 0.0f);
    float4 row3 = float4(rightDirection.z, upDirectionNew.z, forwardDirection.z, 0.0f);
    float4 row4 = float4(-dot(rightDirection, eyePosition), -dot(upDirectionNew, eyePosition), -dot(forwardDirection, eyePosition), 1.0f);

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
        
        float4x4 worldMatrix = RotationMatrix(parameters.RotationX, parameters.RotationY, parameters.RotationZ);
        float4x4 viewMatrix = LookAtLHMatrix(float3(0, 0, -2), float3(0, 0, 0), float3(0, 1, 0));
        float4x4 projectionMatrix = PerspectiveProjectionMatrix(0.78, parameters.AspectRatio, 0.001);

        float4x4 worldViewProjectionMatrix = projectionMatrix * viewMatrix * worldMatrix;

        vertexOutput.Position = worldViewProjectionMatrix * float4(triangleVertices[groupThreadId], 1);
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
