#include "ElementalTools.h"
#include "SystemMemory.h"

// TODO: Do one for each thread
static MemoryArena MeshletBuilderMemoryArena;

void InitMeshletBuilderMemoryArena()
{
    if (MeshletBuilderMemoryArena.Storage == nullptr)
    {
        MeshletBuilderMemoryArena = SystemAllocateMemoryArena(512 * 1024 * 1024);
    }

    //SystemClearMemoryArena(MeshletBuilderMemoryArena);
}

ElemToolsAPI ElemBuildMeshletResult ElemBuildMeshlets(ElemVertexBuffer vertexBuffer, ElemUInt32Span indexBuffer, const ElemBuildMeshletsOptions* options)
{
    InitMeshletBuilderMemoryArena();

    // TODO: Error messages

    auto stackMemoryArena = SystemGetStackMemoryArena();

    auto indexCount = vertexBuffer.Data.Length / vertexBuffer.VertexSize;
    uint32_t* indexBufferPointer = nullptr;

    if (indexBuffer.Length > 0)
    {
        indexBufferPointer = indexBuffer.Items;
        indexCount = indexBuffer.Length;
    }

    auto vertexRemap = SystemPushArrayZero<uint32_t>(stackMemoryArena, indexCount);
    
    auto vertexCount = meshopt_generateVertexRemap(vertexRemap.Pointer, indexBufferPointer, indexCount, vertexBuffer.Data.Items, indexCount, vertexBuffer.VertexSize);

    auto vertexList = SystemPushArrayZero<uint8_t>(MeshletBuilderMemoryArena, vertexCount * vertexBuffer.VertexSize);
    auto indexList = SystemPushArrayZero<uint32_t>(MeshletBuilderMemoryArena, indexCount);

    meshopt_remapVertexBuffer(vertexList.Pointer, vertexBuffer.Data.Items, indexCount, vertexBuffer.VertexSize, vertexRemap.Pointer);
    meshopt_remapIndexBuffer(indexList.Pointer, indexBufferPointer, indexCount, vertexRemap.Pointer);

    meshopt_optimizeVertexCache(indexList.Pointer, indexList.Pointer, indexCount, vertexCount);
    meshopt_optimizeVertexFetch(vertexList.Pointer, indexList.Pointer, indexCount, vertexList.Pointer, vertexCount, vertexBuffer.VertexSize);

    // TODO: Compute bounding sphere and bounding box

    // TODO: Review the default values
    // TODO: Allow customisation
    uint8_t meshletMaxVertexCount = 64u;
    uint8_t meshletMaxTriangleCount = 64u;
    auto coneWeight = 0.5f;

    auto meshletCount = (uint32_t)meshopt_buildMeshletsBound(indexCount, meshletMaxVertexCount, meshletMaxTriangleCount);

    auto meshOptMeshletList = SystemPushArray<meshopt_Meshlet>(stackMemoryArena, meshletCount);
    auto meshletVertexIndexList = SystemPushArray<uint32_t>(MeshletBuilderMemoryArena, meshletCount * meshletMaxVertexCount);
    auto meshletTriangleIndexListRaw = SystemPushArray<uint8_t>(stackMemoryArena, meshletCount * meshletMaxTriangleCount * 3);
    auto meshletTriangleIndexList = SystemPushArrayZero<uint32_t>(MeshletBuilderMemoryArena, meshletCount * meshletMaxTriangleCount);

    meshletCount = meshopt_buildMeshlets(meshOptMeshletList.Pointer, 
                                         meshletVertexIndexList.Pointer, 
                                         meshletTriangleIndexListRaw.Pointer, 
                                         indexList.Pointer, 
                                         indexCount, 
                                         (const float*)vertexList.Pointer, 
                                         vertexCount, 
                                         vertexBuffer.VertexSize, 
                                         meshletMaxVertexCount, 
                                         meshletMaxTriangleCount, 
                                         coneWeight);


    auto meshletVertexIndexCount = 0u;
    auto meshletTriangleIndexCount = 0u;

    //printf("MeshletCount: %d\n", meshletCount);

    auto meshletList = SystemPushArray<ElemMeshlet>(MeshletBuilderMemoryArena, meshletCount);

    for (uint32_t i = 0; i < meshletCount; i++)
    {
        auto meshlet = meshOptMeshletList[i];

        meshopt_optimizeMeshlet(&meshletVertexIndexList[meshlet.vertex_offset], 
                                &meshletTriangleIndexListRaw[meshlet.triangle_offset], 
                                meshlet.triangle_count, 
                                meshlet.vertex_count);

        /*auto meshletBounds = meshopt_computeMeshletBounds(&meshletVertexIndexList[meshlet.vertex_offset], 
                                                          &meshletTriangleIndexListRaw[meshlet.triangle_offset], 
                                                          meshlet.triangle_count, 
                                                          (const float*)vertexList.Pointer, 
                                                          vertexCount, 
                                                          vertexBuffer.VertexSize);*/

        // TODO: Cone and sphere

        meshletList[i] =
        {
            .VertexIndexOffset = meshlet.vertex_offset,
            .VertexIndexCount = meshlet.vertex_count,
            .TriangleOffset = meshlet.triangle_offset / 3,
            .TriangleCount = meshlet.triangle_count
        };

        for (uint32_t j = 0; j < meshlet.triangle_count; j++)
        {
            auto pointer = &meshletTriangleIndexListRaw[meshlet.triangle_offset + j * 3]; 
            
            auto p0 = pointer[0];
            auto p1 = pointer[1];
            auto p2 = pointer[2];

            meshletTriangleIndexList[meshletList[i].TriangleOffset + j] = ((uint32_t)p2) << 16 | ((uint32_t)p1) << 8 | (uint32_t)p0;
        }

        meshletVertexIndexCount += meshlet.vertex_count;
        meshletTriangleIndexCount = fmax(meshletTriangleIndexCount, meshlet.triangle_offset / 3 + meshlet.triangle_count);
    }

    // TODO: Output everything in separate memory arena
    return 
    {
        .MeshletMaxVertexCount = meshletMaxVertexCount,
        .MeshletMaxTriangleCount = meshletMaxTriangleCount,
        .VertexBuffer = { .Data = { .Items = vertexList.Pointer, .Length = (uint32_t)vertexList.Length }, .VertexSize = vertexBuffer.VertexSize, .VertexCount = vertexBuffer.VertexCount },
        .IndexBuffer = { .Items = indexList.Pointer, .Length = (uint32_t)indexList.Length },
        .Meshlets = { .Items = meshletList.Pointer, .Length = (uint32_t)meshletList.Length },
        .MeshletVertexIndexBuffer = { .Items = meshletVertexIndexList.Pointer, .Length = meshletVertexIndexCount },
        .MeshletTriangleIndexBuffer = { .Items = meshletTriangleIndexList.Pointer, .Length = meshletTriangleIndexCount }
    };
}
