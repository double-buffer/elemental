#include "ElementalTools.h"
#include "SampleUtils.h"
#include "SampleMesh.h"

int main(int argc, const char* argv[]) 
{
    // TODO: Add an option to handle handness change
    if (argc < 3)
    {
        printf("USAGE: MeshCompiler [options] inputfile outputfile\n");
        printf("\n");
        printf("OPTIONS:\n");
        printf("   --meshlet-triangle-count\tTBD: TBD. Default: TBD.\n");
        printf("\n");
        return 0;
    }

    int32_t inputPathIndex = argc - 2;
    const char* inputPath = argv[inputPathIndex];

    int32_t outputPathIndex = argc - 1;
    const char* outputPath = argv[outputPathIndex];

    // TODO: Add more checks
    for (uint32_t i = 1; i < (uint32_t)(argc - 2); i++)
    {
        printf("Options: %s\n", argv[i]);

        /*if (strcmp(argv[i], "--target-platform") == 0)
        {
            const char* targetPlatformString = argv[i + 1];

            if (strcmp(targetPlatformString, "iOS") == 0)
            {
                targetPlatform = ElemToolsPlatform_iOS;
                printf("iOS platform\n");
            }
        }
        else if (strcmp(argv[i], "--target-api") == 0)
        {
            const char* targetPlatformString = argv[i + 1];

            if (strcmp(targetPlatformString, "vulkan") == 0)
            {
                targetApi = ElemToolsGraphicsApi_Vulkan;
                printf("Vulkan api\n");
            }
        }
        else if (strcmp(argv[i], "--debug") == 0)
        {
            debugMode = true;
        }*/
    }

    printf("Compiling mesh: %s\n", inputPath);

    ElemToolsDataSpan inputData = SampleReadFile(inputPath); 
    
    if (inputData.Length == 0)
    {
        printf("File doesn't exist.\n");
        return 1;
    }

    ElemLoadMeshResult inputMesh = ElemLoadMesh(inputPath, &(ElemLoadMeshOptions) { .MeshCoordinateSystem = ElemMeshCoordinateSystem_LeftHanded });
    printf("Input mesh vertex Count: %d\n", inputMesh.VertexCount);

    ElemBuildMeshletResult result = ElemBuildMeshlets(inputMesh.VertexBuffer, NULL);
    
    // TODO: Refactor this into an util function to display messages with proper colors
    for (uint32_t i = 0; i < result.Messages.Length; i++)
    {
        printf("Compil msg (%d): %s\n", result.Messages.Items[i].Type, result.Messages.Items[i].Message);
    }

    if (result.HasErrors)
    {
        printf("Error while compiling shader!\n");
        return 1;
    }

    // TODO: Good Unit test too!
    for (uint32_t i = 0; i < result.Meshlets.Length; i++)
    {
        ElemMeshlet meshlet = result.Meshlets.Items[i];

        if (i < result.Meshlets.Length - 1) 
        {
            ElemMeshlet nextMeshlet = result.Meshlets.Items[i + 1];

            if (meshlet.TriangleOffset + meshlet.TriangleCount - 1 == nextMeshlet.TriangleOffset)
            {
                printf("Error not last\n");
            }
        }
        else if (meshlet.TriangleOffset + meshlet.TriangleCount - 1 == result.MeshletTriangleIndexBuffer.Length)
        {
            printf("ERROR\n");
        }
    }

    for (uint32_t i = result.MeshletTriangleIndexBuffer.Length - 10; i < result.MeshletTriangleIndexBuffer.Length + 3; i++)
    {
        printf("Trig index: %u\n", result.MeshletTriangleIndexBuffer.Items[i]);
    }
    
    for (uint32_t i = 0; i < result.MeshletTriangleIndexBuffer.Length; i++)
    {
        if (result.MeshletTriangleIndexBuffer.Items[i] == 0)
        {
            printf("Zero Triangle index at: %d\n", i);
        }
    }

    printf("Writing mesh data to: %s\n", outputPath);

    uint32_t meshletBufferSize = result.Meshlets.Length * sizeof(ElemMeshlet);
    uint32_t meshletVertexIndexBufferSize = result.MeshletVertexIndexBuffer.Length * sizeof(uint32_t);

    SampleMeshHeader meshHeader = 
    {
        .FileId = { 'M', 'E', 'S', 'H' },
        .MeshletCount = result.Meshlets.Length,
        .MeshletMaxVertexCount = result.MeshletMaxVertexCount,
        .MeshletMaxTriangleCount = result.MeshletMaxTriangleCount,
        .VertexBufferOffset = sizeof(SampleMeshHeader),
        .VertexBufferSizeInBytes = result.VertexBuffer.Data.Length,
        .MeshletBufferOffset = sizeof(SampleMeshHeader) + result.VertexBuffer.Data.Length,
        .MeshletBufferSizeInBytes = meshletBufferSize,
        .MeshletVertexIndexBufferOffset = sizeof(SampleMeshHeader) + result.VertexBuffer.Data.Length + meshletBufferSize,
        .MeshletVertexIndexBufferSizeInBytes = meshletVertexIndexBufferSize,
        .MeshletTriangleIndexBufferOffset = sizeof(SampleMeshHeader) + result.VertexBuffer.Data.Length + meshletBufferSize + meshletVertexIndexBufferSize,
        .MeshletTriangleIndexBufferSizeInBytes = result.MeshletTriangleIndexBuffer.Length * sizeof(uint32_t)
    };
    
    SampleWriteDataToFile(outputPath, (ElemToolsDataSpan) { .Items = (uint8_t*)&meshHeader, .Length = sizeof(SampleMeshHeader) }, false);
    SampleWriteDataToFile(outputPath, result.VertexBuffer.Data, true);
    SampleWriteDataToFile(outputPath, (ElemToolsDataSpan) { .Items = (uint8_t*)result.Meshlets.Items, .Length = meshletBufferSize}, true);
    SampleWriteDataToFile(outputPath, (ElemToolsDataSpan) { .Items = (uint8_t*)result.MeshletVertexIndexBuffer.Items, .Length = result.MeshletVertexIndexBuffer.Length * sizeof(uint32_t) }, true);
    SampleWriteDataToFile(outputPath, (ElemToolsDataSpan) { .Items = (uint8_t*)result.MeshletTriangleIndexBuffer.Items, .Length = result.MeshletTriangleIndexBuffer.Length * sizeof(uint32_t) }, true);
}
