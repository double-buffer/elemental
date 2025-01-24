#include "SceneLoader.h"
#include "ToolsUtils.h"
#include "ElementalTools.h"
#include "SystemMemory.h"
#include "SystemFunctions.h"

cgltf_result ClgtfFileRead(const struct cgltf_memory_options* memory_options, const struct cgltf_file_options* file_options, const char* path, cgltf_size* size, void** data)
{
    auto objFileData = LoadFileData(path); 

    if (objFileData.Length == 0)
    {
        return cgltf_result_io_error;
    }

    *data = (uint8_t*)objFileData.Pointer;
    *size = objFileData.Length;
    
    return cgltf_result_success;
}

void ClgtfFileRelease(const struct cgltf_memory_options* memory_options, const struct cgltf_file_options* file_options, void* data)
{
}

ElemVertexBuffer ConstructGltfVertexBuffer(MemoryArena memoryArena, const cgltf_primitive* gltfPrimitive, ElemSceneCoordinateSystem coordinateSystem, ElemToolsBoundingBox* boundingBox)
{
    auto stackMemoryArena = SystemGetStackMemoryArena();
    
    // TODO: Config of the vertex components to load
    // TODO: Support vertex compression (eg: float16 for positions?)
    // TODO: Tangent
    auto positionSize = sizeof(float) * 3;
    auto normalSize = sizeof(float) * 3;
    auto tangentSize = sizeof(float) * 4;
    auto textureCoordinatesSize = sizeof(float) * 2;
    auto maxVertexSize = positionSize + normalSize + tangentSize + textureCoordinatesSize;

    // TODO: Temporary
    auto realVertexSize = maxVertexSize;
    auto vertexCount = gltfPrimitive->attributes[0].data->count;

    // TODO: Combine that with the components selection
    auto positionAccessor = cgltf_find_accessor(gltfPrimitive, cgltf_attribute_type_position, 0);
    auto normalAccessor = cgltf_find_accessor(gltfPrimitive, cgltf_attribute_type_normal, 0);
    auto tangentAccessor = cgltf_find_accessor(gltfPrimitive, cgltf_attribute_type_tangent, 0);
    auto textureCoordinatesAccessor = cgltf_find_accessor(gltfPrimitive, cgltf_attribute_type_texcoord, 0);

    // TODO: Rework this
    auto gltfPositionBuffer = SystemPushArray<float>(stackMemoryArena, vertexCount * 3);
    auto gltfNormalBuffer = SystemPushArray<float>(stackMemoryArena, vertexCount * 3);
    auto gltfTangentBuffer = SystemPushArray<float>(stackMemoryArena, vertexCount * 4);
    auto gltfTextureCoordinatesBuffer = SystemPushArray<float>(stackMemoryArena, vertexCount * 2);

    SystemAssert(positionAccessor);
    cgltf_accessor_unpack_floats(positionAccessor, gltfPositionBuffer.Pointer, gltfPositionBuffer.Length);

    if (normalAccessor)
    {
        cgltf_accessor_unpack_floats(normalAccessor, gltfNormalBuffer.Pointer, gltfNormalBuffer.Length);
    }

    if (tangentAccessor)
    {
        cgltf_accessor_unpack_floats(tangentAccessor, gltfTangentBuffer.Pointer, gltfTangentBuffer.Length);
    }

    if (textureCoordinatesAccessor)
    {
        cgltf_accessor_unpack_floats(textureCoordinatesAccessor, gltfTextureCoordinatesBuffer.Pointer, gltfTextureCoordinatesBuffer.Length);
    }

    auto vertexBuffer = SystemPushArray<uint8_t>(memoryArena, vertexCount * maxVertexSize);
    auto currentVertexBufferPointer = vertexBuffer.Pointer;

    for (uint32_t i = 0; i < vertexCount; i++)
    {
        // Position
        for (uint32_t j = 0; j < 3; j++)
        {
            ((float*)currentVertexBufferPointer)[j] = gltfPositionBuffer[i * 3 + j];

            if (coordinateSystem == ElemSceneCoordinateSystem_LeftHanded)
            {
                ((float*)currentVertexBufferPointer)[2] = -((float*)currentVertexBufferPointer)[2];
            }
        }

        AddPointToBoundingBox({ ((float*)currentVertexBufferPointer)[0], ((float*)currentVertexBufferPointer)[1], ((float*)currentVertexBufferPointer)[2]}, boundingBox);
        currentVertexBufferPointer += 3 * sizeof(float);

        // Normal
        for (uint32_t j = 0; j < 3; j++)
        {
            ((float*)currentVertexBufferPointer)[j] = gltfNormalBuffer[i * 3 + j];

            if (coordinateSystem == ElemSceneCoordinateSystem_LeftHanded)
            {
                ((float*)currentVertexBufferPointer)[2] = -((float*)currentVertexBufferPointer)[2];
            }
        }

        currentVertexBufferPointer += 3 * sizeof(float);

        // Tangent
        for (uint32_t j = 0; j < 4; j++)
        {
            ((float*)currentVertexBufferPointer)[j] = gltfTangentBuffer[i * 4 + j];

            if (coordinateSystem == ElemSceneCoordinateSystem_LeftHanded)
            {
                ((float*)currentVertexBufferPointer)[2] = -((float*)currentVertexBufferPointer)[2];
            }
        }

        currentVertexBufferPointer += 4 * sizeof(float);
        
        // Texture coordinates
        for (uint32_t j = 0; j < 2; j++)
        {
            ((float*)currentVertexBufferPointer)[j] = gltfTextureCoordinatesBuffer[i * 2 + j];
        }
    
        currentVertexBufferPointer += 2 * sizeof(float);
    }

    return 
    {
        .Data = { .Items = vertexBuffer.Pointer, .Length = (uint32_t)vertexBuffer.Length },
        .VertexSize = (uint32_t)realVertexSize,
        .VertexCount = (uint32_t)vertexCount
    };
}

ElemUInt32Span ConstructGltfIndexBuffer(MemoryArena memoryArena, const cgltf_primitive* gltfPrimitive, ElemSceneCoordinateSystem coordinateSystem)
{
    auto indexBuffer = SystemPushArray<uint32_t>(memoryArena, gltfPrimitive->indices->count);
	cgltf_accessor_unpack_indices(gltfPrimitive->indices, indexBuffer.Pointer, sizeof(uint32_t), indexBuffer.Length);

    if (coordinateSystem == ElemSceneCoordinateSystem_LeftHanded)
    {
        for (uint32_t i = 0; i < indexBuffer.Length; i += 3)
        {
            uint32_t temp = indexBuffer[i + 1];
            indexBuffer[i + 1] = indexBuffer[i + 2];
            indexBuffer[i + 2] = temp;
        }
    }

    return { .Items = indexBuffer.Pointer, .Length = (uint32_t)indexBuffer.Length }; 
}

ReadOnlySpan<ElemSceneMesh> LoadGltfMeshes(MemoryArena memoryArena, const cgltf_data* data, const ElemLoadSceneOptions* options, ToolsMessageList* messageList)
{
    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto meshes = SystemPushArray<ElemSceneMesh>(memoryArena, data->meshes_count);

    for (uint32_t i = 0; i < data->meshes_count; i++)
    {
        auto mesh = &meshes[i];
        auto gltfMesh = &data->meshes[i];

        mesh->BoundingBox = 
        { 
            .MinPoint = { FLT_MAX, FLT_MAX, FLT_MAX }, 
            .MaxPoint = { -FLT_MAX, -FLT_MAX, -FLT_MAX } 
        }; 
        
        auto meshPrimitives = SystemPushArray<ElemSceneMeshPrimitive>(memoryArena, gltfMesh->primitives_count);

        for (uint32_t j = 0; j < gltfMesh->primitives_count; j++)
        {
            auto meshPrimitive = &meshPrimitives[j];
            auto gltfPrimitive = &gltfMesh->primitives[j];

            if (gltfPrimitive->type != cgltf_primitive_type_triangles)
            {
                WriteToMessageList(ElemToolsMessageType_Warning, "glTF loader only support triangles for now.", messageList);
                continue;
            }

            if (!cgltf_find_accessor(gltfPrimitive, cgltf_attribute_type_position, 0))
            {
                WriteToMessageList(ElemToolsMessageType_Warning, "The mesh primitive doesn't contains a position attribute.", messageList);
                continue;
            }

            meshPrimitive->BoundingBox = 
            { 
                .MinPoint = { FLT_MAX, FLT_MAX, FLT_MAX }, 
                .MaxPoint = { -FLT_MAX, -FLT_MAX, -FLT_MAX } 
            }; 

            meshPrimitive->VertexBuffer = ConstructGltfVertexBuffer(memoryArena, gltfPrimitive, options->CoordinateSystem, &meshPrimitive->BoundingBox);
            meshPrimitive->IndexBuffer = ConstructGltfIndexBuffer(memoryArena, gltfPrimitive, options->CoordinateSystem);

            meshPrimitive->MaterialId = gltfPrimitive->material ? cgltf_material_index(data, gltfPrimitive->material) : -1;

            AddBoundingBoxToBoundingBox(&meshPrimitive->BoundingBox, &mesh->BoundingBox);
        }

        if (gltfMesh->name)
        {
            mesh->Name = SystemDuplicateBuffer(memoryArena, ReadOnlySpan<char>(gltfMesh->name)).Pointer;
        }
        else
        {
            mesh->Name = SystemConcatBuffers<char>(memoryArena, "Mesh_", SystemConvertNumberToString(stackMemoryArena, i)).Pointer;
        }

        mesh->MeshPrimitives = { .Items = meshPrimitives.Pointer, .Length = (uint32_t)meshPrimitives.Length };
    }

    return meshes;
}

ReadOnlySpan<ElemSceneMaterial> LoadGltfMaterials(MemoryArena memoryArena, const cgltf_data* data, const ElemLoadSceneOptions* options, ToolsMessageList* messageList)
{
    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto materials = SystemPushArray<ElemSceneMaterial>(memoryArena, data->materials_count);

    for (uint32_t i = 0; i < data->materials_count; i++)
    {
        auto material = &materials[i];
        auto gltfMaterial = &data->materials[i];

        if (gltfMaterial->name)
        {
            material->Name = SystemDuplicateBuffer(memoryArena, ReadOnlySpan<char>(gltfMaterial->name)).Pointer;
        }
        else
        {
            material->Name = SystemConcatBuffers<char>(memoryArena, "Material_", SystemConvertNumberToString(stackMemoryArena, i)).Pointer;
        }

        material->EmissiveFactor =
        {
            .X = gltfMaterial->emissive_factor[0],
            .Y = gltfMaterial->emissive_factor[1],
            .Z = gltfMaterial->emissive_factor[2]
        };

        if (gltfMaterial->has_emissive_strength)
        {
            material->EmissiveFactor.X *= gltfMaterial->emissive_strength.emissive_strength;
            material->EmissiveFactor.Y *= gltfMaterial->emissive_strength.emissive_strength;
            material->EmissiveFactor.Z *= gltfMaterial->emissive_strength.emissive_strength;
        }

        if (gltfMaterial->has_pbr_metallic_roughness)
        {
            auto gltfMaterialData = &gltfMaterial->pbr_metallic_roughness;

            if (gltfMaterialData->base_color_texture.texture)
            {
                material->AlbedoTexturePath = SystemDuplicateBuffer<char>(memoryArena, gltfMaterialData->base_color_texture.texture->image->uri).Pointer;
            }

            material->AlbedoFactor = 
            { 
                .X = gltfMaterialData->base_color_factor[0], 
                .Y = gltfMaterialData->base_color_factor[1], 
                .Z = gltfMaterialData->base_color_factor[2], 
                .W = gltfMaterialData->base_color_factor[3] 
            };
        }
        else if (gltfMaterial->has_pbr_specular_glossiness)
        {
            auto gltfMaterialData = &gltfMaterial->pbr_specular_glossiness;

            if (gltfMaterialData->diffuse_texture.texture)
            {
                material->AlbedoTexturePath = SystemDuplicateBuffer<char>(memoryArena, gltfMaterialData->diffuse_texture.texture->image->uri).Pointer;
            }

            material->AlbedoFactor = 
            { 
                .X = gltfMaterialData->diffuse_factor[0], 
                .Y = gltfMaterialData->diffuse_factor[1], 
                .Z = gltfMaterialData->diffuse_factor[2], 
                .W = gltfMaterialData->diffuse_factor[3] 
            };
        }
        else
        {
            WriteToMessageList(ElemToolsMessageType_Warning, "glTF loader only support metallic roughness materials for now.", messageList);
        }

        if (gltfMaterial->normal_texture.texture)
        {
            material->NormalTexturePath = SystemDuplicateBuffer<char>(memoryArena, gltfMaterial->normal_texture.texture->image->uri).Pointer;
        }

        printf("Material: %s\n", material->Name);
    }

    return materials;
}

ReadOnlySpan<ElemSceneNode> LoadGltfSceneNodes(MemoryArena memoryArena, const cgltf_data* data, const ElemLoadSceneOptions* options, ToolsMessageList* messageList)
{
    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto sceneNodes = SystemPushArray<ElemSceneNode>(memoryArena, data->nodes_count);
    auto globalTransformMatrix = CreateSceneLoaderGlobalTransformMatrix(options);

    // TODO: Children?
    for (uint32_t i = 0; i < data->nodes_count; i++)
    {
        auto sceneNode = &sceneNodes[i];
        auto gltfNode = &data->nodes[i];

        if (gltfNode->name)
        {
            sceneNode->Name = SystemDuplicateBuffer(memoryArena, ReadOnlySpan<char>(gltfNode->name)).Pointer;
        }
        else
        {
            sceneNode->Name = SystemConcatBuffers<char>(memoryArena, "Node_", SystemConvertNumberToString(stackMemoryArena, i)).Pointer;
        }

        sceneNode->Rotation = {};
        sceneNode->Translation = {};

        if (gltfNode->mesh)
        {
            sceneNode->NodeType = ElemSceneNodeType_Mesh;
            sceneNode->ReferenceIndex = cgltf_mesh_index(data, gltfNode->mesh);
        }
        
        float matrix[16];
		cgltf_node_transform_world(gltfNode, matrix);
        ElemToolsMatrix4x4 nodeTransform = *(ElemToolsMatrix4x4*)matrix;
    
        if (options->CoordinateSystem == ElemSceneCoordinateSystem_LeftHanded)
        {
            auto flipMatrix = ElemToolsCreateIdentityMatrix();
            flipMatrix.Rows[2].Z = -1;

            nodeTransform = ElemToolsMulMatrix4x4(flipMatrix, ElemToolsMulMatrix4x4(nodeTransform, flipMatrix));
        }

        nodeTransform = ElemToolsMulMatrix4x4(nodeTransform, globalTransformMatrix);
        DecomposeTransform(nodeTransform, &sceneNode->Scale, &sceneNode->Rotation, &sceneNode->Translation);
    }

    return sceneNodes;
}

ElemLoadSceneResult LoadGltfScene(const char* path, const ElemLoadSceneOptions* options)
{
    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto sceneLoaderMemoryArena = GetSceneLoaderMemoryArena();

    ToolsMessageList messageList =
    {
        .Messages = SystemPushArray<ElemToolsMessage>(sceneLoaderMemoryArena, 1024)
    };

    cgltf_options cgltfOptions = 
    {
        .file =
        {
            .read = ClgtfFileRead,
            .release = ClgtfFileRelease
        }
    };

    cgltf_data* data = NULL;
    cgltf_result result = cgltf_parse_file(&cgltfOptions, path, &data);

    if (result != cgltf_result_success)
    {
        return { .Messages = ConstructErrorMessageSpan(sceneLoaderMemoryArena, "Error while reading glTF file."), .HasErrors = true };
    }

    result = cgltf_load_buffers(&cgltfOptions, data, path);

    if (result != cgltf_result_success)
    {
        return { .Messages = ConstructErrorMessageSpan(sceneLoaderMemoryArena, "Error while loading glTF buffers."), .HasErrors = true };
    }

    result = cgltf_validate(data);

    if (result != cgltf_result_success)
    {
        return { .Messages = ConstructErrorMessageSpan(sceneLoaderMemoryArena, "Error while validating glTF file."), .HasErrors = true };
    }
    
    auto meshes = LoadGltfMeshes(sceneLoaderMemoryArena, data, options, &messageList);
    auto sceneNodes = LoadGltfSceneNodes(sceneLoaderMemoryArena, data, options, &messageList);
    auto materials = LoadGltfMaterials(sceneLoaderMemoryArena, data, options, &messageList);
    
    cgltf_free(data);
    ResetLoadFileDataMemory();

    return 
    {
        .SceneFormat = ElemSceneFormat_Gltf,
        .CoordinateSystem = options->CoordinateSystem,
        .Meshes = { .Items = (ElemSceneMesh*)meshes.Pointer, .Length = (uint32_t)meshes.Length },
        .Materials = { .Items = (ElemSceneMaterial*)materials.Pointer, .Length = (uint32_t)materials.Length },
        .Nodes = { .Items = (ElemSceneNode*)sceneNodes.Pointer, .Length = (uint32_t)sceneNodes.Length },
        .Messages = { .Items = messageList.Messages.Pointer, .Length = messageList.MessageCount },
        .HasErrors = messageList.HasErrors
    };
}
