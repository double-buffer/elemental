#include "ElementalTools.h"
#include "SystemMemory.h"

ElemToolsAPI ElemBuildMeshletResult ElemBuildMeshlets(ElemVertexBuffer vertexBuffer, const ElemBuildMeshletsOptions* options)
{
    // TODO: Error messages

    auto stackMemoryArena = SystemGetStackMemoryArena();
    printf("Meshlet Builder\n");

    auto indexCount = vertexBuffer.Data.Length / vertexBuffer.VertexSize;
    auto vertexRemap = SystemPushArray<uint32_t>(stackMemoryArena, indexCount);
    
    auto vertexCount = meshopt_generateVertexRemap(vertexRemap.Pointer, nullptr, indexCount, vertexBuffer.Data.Items, indexCount, vertexBuffer.VertexSize);
    printf("VertexCount after remap: %lu\n", vertexCount);

    auto vertexList = SystemPushArray<uint8_t>(stackMemoryArena, vertexCount * vertexBuffer.VertexSize);
    auto indexList = SystemPushArray<uint32_t>(stackMemoryArena, indexCount);

    meshopt_remapVertexBuffer(vertexList.Pointer, vertexBuffer.Data.Items, indexCount, vertexBuffer.VertexSize, vertexRemap.Pointer);
    meshopt_remapIndexBuffer(indexList.Pointer, nullptr, indexCount, vertexRemap.Pointer);

    meshopt_optimizeVertexCache(indexList.Pointer, indexList.Pointer, indexCount, vertexCount);
    meshopt_optimizeVertexFetch(vertexList.Pointer, indexList.Pointer, indexCount, vertexList.Pointer, vertexCount, vertexBuffer.VertexSize);

    // TODO: Compute bounding sphere and bounding box

    // TODO: Review the default values
    // TODO: Allow customisation
    auto meshletMaxVertexCount = 64;
    auto meshletMaxTriangleCount = 64;
    auto coneWeight = 0.5f;

    auto meshletCount = meshopt_buildMeshletsBound(indexCount, meshletMaxVertexCount, meshletMaxTriangleCount);
    printf("MeshletCount: %d\n", meshletCount);

    auto meshOptMeshletList = SystemPushArray<meshopt_Meshlet>(stackMemoryArena, meshletCount);
    auto meshletVertexIndexList = SystemPushArray<uint32_t>(stackMemoryArena, meshletCount * meshletMaxVertexCount);
    auto meshletTriangleIndexList = SystemPushArray<uint8_t>(stackMemoryArena, meshletCount * meshletMaxTriangleCount);

    meshletCount = meshopt_buildMeshlets(meshOptMeshletList.Pointer, 
                                         meshletVertexIndexList.Pointer, 
                                         meshletTriangleIndexList.Pointer, 
                                         indexList.Pointer, 
                                         indexCount, 
                                         (const float*)vertexList.Pointer, 
                                         vertexCount, 
                                         vertexBuffer.VertexSize, 
                                         meshletMaxVertexCount, 
                                         meshletMaxTriangleCount, 
                                         coneWeight);

    printf("MeshletCount: %d\n", meshletCount);

    auto meshletList = SystemPushArray<ElemMeshlet>(stackMemoryArena, meshletCount);

    for (uint32_t i = 0; i < meshletCount; i++)
    {
        auto meshlet = meshOptMeshletList[i];

        meshopt_optimizeMeshlet(&meshletVertexIndexList[meshlet.vertex_offset], 
                                &meshletTriangleIndexList[meshlet.triangle_offset], 
                                meshlet.triangle_count, 
                                meshlet.vertex_count);

        auto meshletBounds = meshopt_computeMeshletBounds(&meshletVertexIndexList[meshlet.vertex_offset], 
                                                          &meshletTriangleIndexList[meshlet.triangle_offset], 
                                                          meshlet.triangle_count, 
                                                          (const float*)vertexList.Pointer, 
                                                          vertexCount, 
                                                          vertexBuffer.VertexSize);

        // TODO: Cone and sphere

        meshletList[i] =
        {
            .VertexIndexOffset = meshlet.vertex_offset,
            .VertexIndexCount = meshlet.vertex_count,
            .TriangleIndexOffset = meshlet.triangle_offset,
            .TriangleIndexCount = meshlet.vertex_count
        };
    }

    printf("Done\n");

    return 
    {
        .MeshletMaxVertexCount = meshletMaxVertexCount,
        .MeshletMaxTriangleCount = meshletMaxTriangleCount,
        .VertexBuffer = { .Data =  { .Items = vertexList.Pointer, .Length = vertexList.Length }, .VertexSize = vertexBuffer.VertexSize },
        .Meshlets = { .Items = meshletList.Pointer, .Length = meshletList.Length },
        .MeshletVertexIndexBuffer = { .Items = meshletVertexIndexList.Pointer, .Length = meshletVertexIndexList.Length },
        .MeshletTriangleIndexBuffer = { .Items = meshletTriangleIndexList.Pointer, .Length = meshletTriangleIndexList.Length }
    };
}
