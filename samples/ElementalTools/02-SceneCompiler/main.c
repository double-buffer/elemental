#include "ElementalTools.h"
#include "SampleUtils.h"
#include "SampleScene.h"

// TODO: What we could do is to separate the global index buffer and put it at the end of the file
// so we can load the mesh buffer and the index buffer. And then dismiss the index buffer when
// the BLAS has been generated.

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

    SampleMeshPrimitiveHeader* meshPrimitiveHeaders = (SampleMeshPrimitiveHeader*)malloc(sizeof(SampleMeshPrimitiveHeader) * mesh.MeshPrimitives.Length);
    ElemBuildMeshletResult* buildMeshletResults = (ElemBuildMeshletResult*)malloc(sizeof(ElemBuildMeshletResult) * mesh.MeshPrimitives.Length);

    uint32_t vertexBufferSizeInBytes = 0u;
    uint32_t indexBufferSizeInBytes = 0u;
    uint32_t meshletCount = 0u;
    uint32_t meshletVertexIndexCount = 0u;
    uint32_t meshletTriangleIndexCount = 0u;
    uint32_t vertexSizeInBytes = 0u;

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

        buildMeshletResults[i] = result;

        vertexBufferSizeInBytes += result.VertexBuffer.Data.Length;
        indexBufferSizeInBytes += result.IndexBuffer.Length * sizeof(uint32_t);

        meshletCount += result.Meshlets.Length;
        meshletVertexIndexCount += result.MeshletVertexIndexBuffer.Length;
        meshletTriangleIndexCount += result.MeshletTriangleIndexBuffer.Length;
        vertexSizeInBytes = result.VertexBuffer.VertexSize;
    }

    meshHeader.VertexSizeInBytes = vertexSizeInBytes;
    meshHeader.MeshBufferOffset = ftell(file);

    uint32_t meshPrimitivesOffset = 0;
    uint32_t meshVertexBufferOffset = meshPrimitivesOffset + mesh.MeshPrimitives.Length * sizeof(SampleMeshPrimitiveHeader);
    uint32_t meshIndexBufferOffset = meshVertexBufferOffset + vertexBufferSizeInBytes;

    uint32_t meshletBufferOffset = meshIndexBufferOffset + indexBufferSizeInBytes;
    uint32_t meshletVertexIndexBufferOffset = meshletBufferOffset + meshletCount * sizeof(ElemMeshlet);
    uint32_t meshletTriangleIndexBufferOffset = meshletVertexIndexBufferOffset + meshletVertexIndexCount * sizeof(uint32_t);

    meshHeader.MeshBufferSizeInBytes = meshletTriangleIndexBufferOffset + meshletTriangleIndexCount * sizeof(uint32_t);

    uint32_t currentPrimitivesOffset = 0u;
    uint32_t currentVertexBufferOffset = 0u;
    uint32_t currentIndexBufferOffset = 0u;
    uint32_t currentMeshletBufferOffset = 0u;
    uint32_t currentMeshletVertexIndexBufferOffset = 0u;
    uint32_t currentMeshletTriangleIndexBufferOffset = 0u;

    for (uint32_t i = 0; i < mesh.MeshPrimitives.Length; i++)
    {
        ElemSceneMeshPrimitive* meshPrimitive = &mesh.MeshPrimitives.Items[i];

        ElemBuildMeshletResult result = buildMeshletResults[i];

        SampleMeshPrimitiveHeader* meshPrimitiveHeader = &meshPrimitiveHeaders[i];
        meshPrimitiveHeader->MaterialId = meshPrimitive->MaterialId;
        meshPrimitiveHeader->MeshletCount = result.Meshlets.Length;
        // TODO: Other properties?

        uint32_t vertexBufferOffset = meshVertexBufferOffset + currentVertexBufferOffset;
        fseek(file, vertexBufferOffset + meshHeader.MeshBufferOffset, SEEK_SET);
        fwrite(result.VertexBuffer.Data.Items, sizeof(uint8_t), result.VertexBuffer.Data.Length, file);

        meshPrimitiveHeader->VertexBufferOffset = vertexBufferOffset;
        meshPrimitiveHeader->VertexCount = result.VertexBuffer.Data.Length / result.VertexBuffer.VertexSize;

        currentVertexBufferOffset += result.VertexBuffer.Data.Length;
        
        uint32_t indexBufferOffset = meshIndexBufferOffset + currentIndexBufferOffset;
        fseek(file, indexBufferOffset + meshHeader.MeshBufferOffset, SEEK_SET);
        fwrite(result.IndexBuffer.Items, sizeof(uint32_t), result.IndexBuffer.Length, file);

        meshPrimitiveHeader->IndexBufferOffset = indexBufferOffset;
        meshPrimitiveHeader->IndexCount = result.IndexBuffer.Length;
        currentIndexBufferOffset += result.IndexBuffer.Length * sizeof(uint32_t);

        uint32_t meshletVertexIndexOffset = meshletVertexIndexBufferOffset + currentMeshletVertexIndexBufferOffset;
        fseek(file, meshletVertexIndexOffset + meshHeader.MeshBufferOffset, SEEK_SET);
        fwrite(result.MeshletVertexIndexBuffer.Items, sizeof(uint32_t), result.MeshletVertexIndexBuffer.Length, file);
        currentMeshletVertexIndexBufferOffset += result.MeshletVertexIndexBuffer.Length * sizeof(uint32_t);

        uint32_t meshletTriangleIndexOffset = meshletTriangleIndexBufferOffset + currentMeshletTriangleIndexBufferOffset;
        fseek(file, meshletTriangleIndexOffset + meshHeader.MeshBufferOffset, SEEK_SET);
        fwrite(result.MeshletTriangleIndexBuffer.Items, sizeof(uint32_t), result.MeshletTriangleIndexBuffer.Length, file);
        currentMeshletTriangleIndexBufferOffset += result.MeshletTriangleIndexBuffer.Length * sizeof(uint32_t);

        for (uint32_t j = 0; j < result.Meshlets.Length; j++)
        {
            ElemMeshlet* meshlet = &result.Meshlets.Items[j];
            meshlet->VertexIndexOffset = meshletVertexIndexOffset + meshlet->VertexIndexOffset * sizeof(uint32_t);
            meshlet->TriangleOffset = meshletTriangleIndexOffset + meshlet->TriangleOffset * sizeof(uint32_t);
        }

        meshPrimitiveHeader->MeshletOffset = meshletBufferOffset + currentMeshletBufferOffset;
        fseek(file, meshPrimitiveHeader->MeshletOffset + meshHeader.MeshBufferOffset, SEEK_SET);
        fwrite(result.Meshlets.Items, sizeof(ElemMeshlet), result.Meshlets.Length, file);
        currentMeshletBufferOffset += result.Meshlets.Length * sizeof(ElemMeshlet);
        
        uint32_t primitivesOffset = meshPrimitivesOffset + currentPrimitivesOffset;
        fseek(file, primitivesOffset + meshHeader.MeshBufferOffset, SEEK_SET);
        fwrite(meshPrimitiveHeader, sizeof(SampleMeshPrimitiveHeader), 1, file);
        currentPrimitivesOffset += sizeof(SampleMeshPrimitiveHeader);
    }

    fseek(file, meshHeaderOffset, SEEK_SET);
    fwrite(&meshHeader, sizeof(SampleMeshHeader), 1, file);
    
    fseek(file, 0, SEEK_END);
    
    free(buildMeshletResults);

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
            .AlbedoFactor = material->AlbedoFactor,
            .EmissiveFactor = material->EmissiveFactor
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
    if (strstr(inputPath, "sponza.gltf"))
    {
    }
    else if (strstr(inputPath, "sponza.obj"))
    {
        loadSceneOptions.Scaling = 0.01f;
        //loadSceneOptions.CoordinateSystem = ElemSceneCoordinateSystem_RightHanded;
    }
    else if (strstr(inputPath, "bistro"))
    {
        //loadSceneOptions.CoordinateSystem = ElemSceneCoordinateSystem_RightHanded;
    }
    
    // TODO: Scaling should be passed as a parameter
    ElemLoadSceneResult scene = ElemLoadScene(inputPath, &loadSceneOptions);
    
    printf("Scene Loaded: Meshes Count=%d, Material Count=%d, Nodes Count=%d\n", scene.Meshes.Length, scene.Materials.Length, scene.Nodes.Length);
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
