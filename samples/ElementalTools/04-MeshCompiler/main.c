#include "ElementalTools.h"
#include "SampleUtils.h"

#define MESH_FILE_VERSION 1u
#define MESH_VERTEX_SIZE_IN_BYTES (sizeof(float) * 12u)

typedef struct
{
    char FileId[4];
    uint32_t Version;
    uint32_t MeshBufferSizeInBytes;
    uint32_t VertexSizeInBytes;
    uint32_t VertexBufferOffset;
    uint32_t MeshletOffset;
    uint32_t MeshletVertexIndexOffset;
    uint32_t MeshletTriangleIndexOffset;
    uint32_t MeshletCount;
} MeshFileHeader;

bool WriteData(FILE* file, const void* data, uint32_t sizeInBytes)
{
    return sizeInBytes == 0 || fwrite(data, 1, sizeInBytes, file) == sizeInBytes;
}

bool WriteMeshFile(const char* outputPath, ElemBuildMeshletResult meshletResult)
{
    uint32_t vertexBufferSizeInBytes = meshletResult.VertexBuffer.Data.Length;
    uint32_t meshletBufferSizeInBytes = meshletResult.Meshlets.Length * sizeof(ElemMeshlet);
    uint32_t meshletVertexIndexBufferSizeInBytes = meshletResult.MeshletVertexIndexBuffer.Length * sizeof(uint32_t);
    uint32_t meshletTriangleIndexBufferSizeInBytes = meshletResult.MeshletTriangleIndexBuffer.Length * sizeof(uint32_t);

    MeshFileHeader header =
    {
        .FileId = { 'M', 'E', 'S', 'H' },
        .Version = MESH_FILE_VERSION,
        .VertexSizeInBytes = meshletResult.VertexBuffer.VertexSize,
        .VertexBufferOffset = 0,
        .MeshletOffset = vertexBufferSizeInBytes,
        .MeshletVertexIndexOffset = vertexBufferSizeInBytes + meshletBufferSizeInBytes,
        .MeshletTriangleIndexOffset = vertexBufferSizeInBytes + meshletBufferSizeInBytes + meshletVertexIndexBufferSizeInBytes,
        .MeshletCount = meshletResult.Meshlets.Length
    };

    header.MeshBufferSizeInBytes = header.MeshletTriangleIndexOffset + meshletTriangleIndexBufferSizeInBytes;

    FILE* file = fopen(outputPath, "wb");

    if (!file)
    {
        printf("Unable to open output mesh: %s\n", outputPath);
        return false;
    }

    bool result =
        fwrite(&header, sizeof(header), 1, file) == 1 &&
        WriteData(file, meshletResult.VertexBuffer.Data.Items, vertexBufferSizeInBytes) &&
        WriteData(file, meshletResult.Meshlets.Items, meshletBufferSizeInBytes) &&
        WriteData(file, meshletResult.MeshletVertexIndexBuffer.Items, meshletVertexIndexBufferSizeInBytes) &&
        WriteData(file, meshletResult.MeshletTriangleIndexBuffer.Items, meshletTriangleIndexBufferSizeInBytes);

    fclose(file);
    return result;
}

int main(int argc, const char* argv[])
{
    if (argc != 3)
    {
        printf("USAGE: MeshCompiler inputfile outputfile\n");
        return 0;
    }

    const char* inputPath = argv[1];
    const char* outputPath = argv[2];

    printf("Loading mesh: %s\n", inputPath);

    ElemLoadSceneResult scene = ElemLoadScene(inputPath, NULL);
    DisplayOutputMessages("LoadScene", scene.Messages);

    if (scene.HasErrors)
    {
        return 1;
    }

    if (scene.Meshes.Length == 0 || scene.Meshes.Items[0].MeshPrimitives.Length == 0)
    {
        printf("The source file does not contain a mesh primitive.\n");
        return 1;
    }

    if (scene.Meshes.Length > 1 || scene.Meshes.Items[0].MeshPrimitives.Length > 1)
    {
        printf("MeshCompiler uses only the first mesh primitive in the source file.\n");
    }

    ElemSceneMeshPrimitive meshPrimitive = scene.Meshes.Items[0].MeshPrimitives.Items[0];
    ElemBuildMeshletResult meshletResult = ElemBuildMeshlets(meshPrimitive.VertexBuffer, meshPrimitive.IndexBuffer, NULL);
    DisplayOutputMessages("BuildMeshlets", meshletResult.Messages);

    if (meshletResult.HasErrors)
    {
        return 1;
    }

    if (meshletResult.VertexBuffer.VertexSize != MESH_VERTEX_SIZE_IN_BYTES)
    {
        printf("Unsupported vertex size: %u bytes (HelloMesh expects %zu bytes).\n", meshletResult.VertexBuffer.VertexSize, MESH_VERTEX_SIZE_IN_BYTES);
        return 1;
    }

    if (!WriteMeshFile(outputPath, meshletResult))
    {
        printf("Unable to write mesh: %s\n", outputPath);
        return 1;
    }

    printf("Compiled %u vertices and %u meshlets to %s\n",
           meshletResult.VertexBuffer.VertexCount,
           meshletResult.Meshlets.Length,
           outputPath);

    return 0;
}
