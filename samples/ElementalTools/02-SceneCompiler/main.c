#include "ElementalTools.h"
#include "SampleUtils.h"
#include "SampleScene.h"

// TODO: Restore MeshBuilder to use for hello mesh?

bool WriteMeshData(FILE* file, ElemSceneMesh mesh)
{
    assert(file);
    
    SampleMeshHeader meshHeader =
    {
        .MeshPartCount = mesh.MeshParts.Length
    };

    fwrite(&meshHeader, sizeof(SampleMeshHeader), 1, file);
    uint32_t meshPartHeadersOffset = ftell(file);
    
    SampleMeshPartHeader* meshPartHeaders = (SampleMeshPartHeader*)malloc(sizeof(SampleMeshPartHeader) * mesh.MeshParts.Length);
    fwrite(meshPartHeaders, sizeof(SampleMeshPartHeader), mesh.MeshParts.Length, file);

    uint32_t meshPartBufferStartOffset = ftell(file);

    for (uint32_t i = 0; i < mesh.MeshParts.Length; i++)
    {
        ElemSceneMeshPart* meshPart = &mesh.MeshParts.Items[i];

        // TODO: Index buffer
        ElemBuildMeshletResult result = ElemBuildMeshlets(meshPart->VertexBuffer, NULL);

        DisplayOutputMessages("BuildMeshlets", result.Messages);

        if (result.HasErrors)
        {
            return false; 
        }

        SampleMeshPartHeader* meshPartHeader = &meshPartHeaders[i];
        meshPartHeader->MeshletCount = result.Meshlets.Length;

        meshPartHeader->VertexBufferOffset = ftell(file) - meshPartBufferStartOffset;
        fwrite(result.VertexBuffer.Data.Items, sizeof(uint8_t), result.VertexBuffer.Data.Length, file);

        meshPartHeader->MeshletOffset = ftell(file) - meshPartBufferStartOffset;
        fwrite(result.Meshlets.Items, sizeof(ElemMeshlet), result.Meshlets.Length, file);

        meshPartHeader->MeshletVertexIndexOffset = ftell(file) - meshPartBufferStartOffset;
        fwrite(result.MeshletVertexIndexBuffer.Items, sizeof(uint32_t), result.MeshletVertexIndexBuffer.Length, file);

        meshPartHeader->MeshletTriangleIndexOffset = ftell(file) - meshPartBufferStartOffset;
        fwrite(result.MeshletTriangleIndexBuffer.Items, sizeof(uint32_t), result.MeshletTriangleIndexBuffer.Length, file);
    }
    
    fseek(file, meshPartHeadersOffset, SEEK_SET);
    fwrite(meshPartHeaders, sizeof(SampleMeshPartHeader), mesh.MeshParts.Length, file);

    fseek(file, 0, SEEK_END);
    
    return true;
}

bool WriteSceneData(FILE* file, ElemLoadSceneResult scene)
{
    assert(file);

    SampleSceneHeader sceneHeader =
    {
        .FileId = { 'S', 'C', 'E', 'N', 'E' },
        .MeshCount = scene.Meshes.Length,
    };

    fwrite(&sceneHeader, sizeof(SampleSceneHeader), 1, file);

    // TODO: Get rid of malloc?

    // TODO: change the uint32_t to a struct of offset + size

    SampleDataBlockEntry* meshDataOffsets = (SampleDataBlockEntry*)malloc(sizeof(SampleDataBlockEntry) * scene.Meshes.Length);
    fwrite(meshDataOffsets, sizeof(SampleDataBlockEntry), scene.Meshes.Length, file);

    double beforeMeshlets = SampleGetTimerValueInMS();

    for (uint32_t i = 0; i < scene.Meshes.Length; i++)
    {
        uint32_t dataOffset = ftell(file);

        bool result = WriteMeshData(file, scene.Meshes.Items[i]);
        assert(result);

        meshDataOffsets[i] = (SampleDataBlockEntry) { .Offset = dataOffset, .SizeInBytes = ftell(file) - dataOffset };
        printf("MeshDataOffset: %d, size=%d\n", meshDataOffsets[i].Offset, meshDataOffsets[i].SizeInBytes);
    }

    printf("Built meshlets in %.2fs\n", (SampleGetTimerValueInMS() - beforeMeshlets) / 1000.0);

    fseek(file, sizeof(sceneHeader), SEEK_SET);
    fwrite(meshDataOffsets, sizeof(SampleDataBlockEntry), scene.Meshes.Length, file);

    return true;
}

int main(int argc, const char* argv[]) 
{
    // TODO: Add an option to handle handness change
    if (argc < 3)
    {
        printf("USAGE: SceneCompiler [options] inputfile outputfile\n");
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

    SampleInitTimer();
    double initialTimer = SampleGetTimerValueInMS();

    ElemToolsDataSpan inputData = SampleReadFile(inputPath, false); 
    
    if (inputData.Length == 0)
    {
        printf("File doesn't exist.\n");
        return 1;
    }

    ElemLoadSceneResult scene = ElemLoadScene(inputPath, &(ElemLoadSceneOptions) { .CoordinateSystem = ElemSceneCoordinateSystem_LeftHanded });
    
    DisplayOutputMessages("LoadScene", scene.Messages);

    if (scene.HasErrors)
    {
        return 1;
    }

    printf("Loaded mesh in %.2fs\n", (SampleGetTimerValueInMS() - initialTimer) / 1000.0);

    printf("Writing Scene data to: %s\n", outputPath);

    FILE* file = fopen(outputPath, "wb");
    WriteSceneData(file, scene);
    fclose(file);
    printf("Scene compiled in %.2fs\n", (SampleGetTimerValueInMS() - initialTimer) / 1000.0);
}
