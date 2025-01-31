#include "ElementalTools.h"
#include "SampleUtils.h"
#include "SampleScene.h"

// TODO: Restore MeshBuilder to use for hello mesh?

bool WriteMeshData(FILE* file, ElemSceneMesh mesh)
{
    assert(file);
    uint32_t meshHeaderOffset = ftell(file);
    
    SampleMeshHeader meshHeader =
    {
        .MeshPrimitiveCount = mesh.MeshPrimitives.Length
    };
    
    strncpy(meshHeader.Name, mesh.Name, 50);

    fwrite(&meshHeader, sizeof(SampleMeshHeader), 1, file);
    uint32_t meshPrimitiveHeadersOffset = ftell(file);
    
    SampleMeshPrimitiveHeader* meshPrimitiveHeaders = (SampleMeshPrimitiveHeader*)malloc(sizeof(SampleMeshPrimitiveHeader) * mesh.MeshPrimitives.Length);
    fwrite(meshPrimitiveHeaders, sizeof(SampleMeshPrimitiveHeader), mesh.MeshPrimitives.Length, file);

    meshHeader.MeshBufferOffset = ftell(file);

    for (uint32_t i = 0; i < mesh.MeshPrimitives.Length; i++)
    {
        ElemSceneMeshPrimitive* meshPrimitive = &mesh.MeshPrimitives.Items[i];

        // TODO: LOD!
        ElemBuildMeshletResult result = ElemBuildMeshlets(meshPrimitive->VertexBuffer, meshPrimitive->IndexBuffer, NULL);

        DisplayOutputMessages("BuildMeshlets", result.Messages);

        if (result.HasErrors)
        {
            return false; 
        }

        SampleMeshPrimitiveHeader* meshPrimitiveHeader = &meshPrimitiveHeaders[i];
        meshPrimitiveHeader->MaterialId = meshPrimitive->MaterialId;
        meshPrimitiveHeader->MeshletCount = result.Meshlets.Length;

        meshPrimitiveHeader->VertexBufferOffset = ftell(file) - meshHeader.MeshBufferOffset;
        fwrite(result.VertexBuffer.Data.Items, sizeof(uint8_t), result.VertexBuffer.Data.Length, file);

        meshPrimitiveHeader->MeshletOffset = ftell(file) - meshHeader.MeshBufferOffset;
        fwrite(result.Meshlets.Items, sizeof(ElemMeshlet), result.Meshlets.Length, file);

        meshPrimitiveHeader->MeshletVertexIndexOffset = ftell(file) - meshHeader.MeshBufferOffset;
        fwrite(result.MeshletVertexIndexBuffer.Items, sizeof(uint32_t), result.MeshletVertexIndexBuffer.Length, file);

        meshPrimitiveHeader->MeshletTriangleIndexOffset = ftell(file) - meshHeader.MeshBufferOffset;
        fwrite(result.MeshletTriangleIndexBuffer.Items, sizeof(uint32_t), result.MeshletTriangleIndexBuffer.Length, file);
    }

    meshHeader.MeshBufferSizeInBytes = ftell(file) - meshHeader.MeshBufferOffset;

    fseek(file, meshHeaderOffset, SEEK_SET);
    fwrite(&meshHeader, sizeof(SampleMeshHeader), 1, file);
    
    fseek(file, meshPrimitiveHeadersOffset, SEEK_SET);
    fwrite(meshPrimitiveHeaders, sizeof(SampleMeshPrimitiveHeader), mesh.MeshPrimitives.Length, file);

    fseek(file, 0, SEEK_END);
    
    return true;
}

bool WriteSceneData(FILE* file, ElemLoadSceneResult scene, const char* sceneInputPath)
{
    assert(file);

    SampleSceneHeader sceneHeader =
    {
        .FileId = { 'S', 'C', 'E', 'N', 'E' },
        .MeshCount = scene.Meshes.Length,
        .MaterialCount = scene.Materials.Length,
        .NodeCount = scene.Nodes.Length
    };

    fwrite(&sceneHeader, sizeof(SampleSceneHeader), 1, file);

    // TODO: Get rid of malloc?
    SampleDataBlockEntry* meshDataOffsets = (SampleDataBlockEntry*)malloc(sizeof(SampleDataBlockEntry) * scene.Meshes.Length);
    fwrite(meshDataOffsets, sizeof(SampleDataBlockEntry), scene.Meshes.Length, file);
    
    for (uint32_t i = 0; i < scene.Materials.Length; i++)
    {
        ElemSceneMaterial* material = &scene.Materials.Items[i];

        SampleSceneMaterialHeader materialHeader =
        {
            .AlbedoFactor = material->AlbedoFactor
        };

        strncpy(materialHeader.Name, material->Name, 50);

        // TODO: Here we need to compute a list of unique textures and add them to a list
        // so we don't need to compute that unique list at runtime
        if (material->AlbedoTexturePath)
        {
            GetRelativeResourcePath(sceneInputPath, material->AlbedoTexturePath, ".texture", materialHeader.AlbedoTexturePath, 255);
        }
        
        if (material->NormalTexturePath)
        {
            GetRelativeResourcePath(sceneInputPath, material->NormalTexturePath, ".texture", materialHeader.NormalTexturePath, 255);
        }

        fwrite(&materialHeader, sizeof(SampleSceneMaterialHeader), 1, file);
    }

    for (uint32_t i = 0; i < scene.Nodes.Length; i++)
    {
        ElemSceneNode* node = &scene.Nodes.Items[i];

        SampleSceneNodeHeader fileNode =
        {
            .NodeType = (SampleSceneNodeType)node->NodeType,
            .ReferenceIndex = node->ReferenceIndex,
            .Rotation = node->Rotation,
            .Scale = node->Scale,
            .Translation = node->Translation
        };

        strncpy(fileNode.Name, node->Name, 50);
        fwrite(&fileNode, sizeof(SampleSceneNodeHeader), 1, file);

        //printf("Node: '%s' MeshId=%d (T=%f,%f,%f)\n", node->Name, node->ReferenceIndex, node->Translation.X, node->Translation.Y, node->Translation.Z);
    }

    double beforeMeshlets = SampleGetTimerValueInMS();

    for (uint32_t i = 0; i < scene.Meshes.Length; i++)
    {
        uint32_t dataOffset = ftell(file);

        bool result = WriteMeshData(file, scene.Meshes.Items[i]);
        assert(result);

        meshDataOffsets[i] = (SampleDataBlockEntry) { .Offset = dataOffset, .SizeInBytes = ftell(file) - dataOffset };
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

    ElemLoadSceneOptions loadSceneOptions = {};

    // HACK: For now we hardcode options
    if (strstr(inputPath, "sponza"))
    {
        loadSceneOptions.Scaling = 0.01f;
    }
    else if (strstr(inputPath, "Sponza.gltf"))
    {
        //loadSceneOptions.CoordinateSystem = ElemSceneCoordinateSystem_RightHanded;
    }
    else if (strstr(inputPath, "bistro"))
    {
        //loadSceneOptions.CoordinateSystem = ElemSceneCoordinateSystem_RightHanded;
    }
    
    // TODO: Scaling should be passed as a parameter
    ElemLoadSceneResult scene = ElemLoadScene(inputPath, &loadSceneOptions);
    
    DisplayOutputMessages("LoadScene", scene.Messages);

    if (scene.HasErrors)
    {
        return 1;
    }

    printf("Loaded mesh in %.2fs\n", (SampleGetTimerValueInMS() - initialTimer) / 1000.0);

    printf("Writing Scene data to: %s\n", outputPath);

    FILE* file = fopen(outputPath, "wb");
    WriteSceneData(file, scene, inputPath);
    fclose(file);
    printf("Scene compiled in %.2fs\n", (SampleGetTimerValueInMS() - initialTimer) / 1000.0);
}
