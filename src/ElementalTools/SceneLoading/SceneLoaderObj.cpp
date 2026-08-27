#include "SceneLoader.h"
#include "ToolsUtils.h"
#include "TangentSpaceGenerator.h"
#include "ElementalTools.h"
#include "SystemMemory.h"
#include "SystemFunctions.h"

struct ObjLoaderFileData
{
    ReadOnlySpan<uint8_t> FileData;
    uint32_t CurrentOffset;
};

struct ObjMeshPrimitiveInfo
{
    uint32_t VertexOffset;
    uint32_t VertexCount; 
    uint32_t FaceOffset;
    uint32_t FaceCount;
    int32_t MaterialId;
};

struct ObjVertex
{
    ElemToolsVector3 Position;
    ElemToolsVector3 Normal;
    ElemToolsVector4 Tangent;
    ElemToolsVector3 Bitangent;
    ElemToolsVector2 TextureCoordinates;
};

void* FastObjFileOpen(const char* path, void* userData)
{
    // TODO: Add support for streaming
    auto objFileData = LoadFileData(path); 

    if (objFileData.Length == 0)
    {
        return nullptr;
    }

    auto objLoaderFileData = SystemPushStruct<ObjLoaderFileData>(*(MemoryArena*)userData);
    objLoaderFileData->CurrentOffset = 0;
    objLoaderFileData->FileData = objFileData;
    
    return objLoaderFileData;
}

void FastObjFileClose(void* file, void* userData)
{
}

size_t FastObjFileRead(void* file, void* destination, size_t bytes, void* userData)
{
    SystemAssert(file);

    auto objLoaderFileData = (ObjLoaderFileData*)file;
    auto sourceSpan = objLoaderFileData->FileData;
    auto destinationSpan = Span<uint8_t>((uint8_t*)destination, bytes);

    auto readLength = SystemMin(sourceSpan.Length - objLoaderFileData->CurrentOffset, bytes);

    if (readLength > 0)
    {
        SystemCopyBuffer(destinationSpan, sourceSpan.Slice(objLoaderFileData->CurrentOffset, readLength));
    }

    objLoaderFileData->CurrentOffset += readLength;
    return readLength;
}

unsigned long FastObjFileSize(void* file, void* userData)
{
    SystemAssert(file);

    auto objLoaderFileData = (ObjLoaderFileData*)file;
    return objLoaderFileData->FileData.Length;
}


ObjVertex ReadObjVertex(fastObjIndex objVertex, const fastObjMesh* objMesh, ElemSceneCoordinateSystem coordinateSystem, bool flipVerticalTextureCoordinates)
{
    ObjVertex result = {};

    if (objVertex.p)
    {
        result.Position.X = objMesh->positions[3 * objVertex.p];
        result.Position.Y = objMesh->positions[3 * objVertex.p + 1];
        result.Position.Z = objMesh->positions[3 * objVertex.p + 2];

        if (coordinateSystem == ElemSceneCoordinateSystem_LeftHanded)
        {
            result.Position.Z = -result.Position.Z;
        }
    }

    if (objVertex.n)
    {
        result.Normal.X = objMesh->normals[3 * objVertex.n];
        result.Normal.Y = objMesh->normals[3 * objVertex.n + 1];
        result.Normal.Z = objMesh->normals[3 * objVertex.n + 2];

        if (coordinateSystem == ElemSceneCoordinateSystem_LeftHanded)
        {
            result.Normal.Z = -result.Normal.Z;
        }
    }

    if (objVertex.t)
    {
        result.TextureCoordinates.X = objMesh->texcoords[2 * objVertex.t];
        result.TextureCoordinates.Y = objMesh->texcoords[2 * objVertex.t + 1];

        if (!flipVerticalTextureCoordinates)
        {
            result.TextureCoordinates.Y = 1 - result.TextureCoordinates.Y;
        }
    }

    return result;
}

// TODO: Pass vertex format configured by the lib
void WriteObjVertex(const ObjVertex* vertex, uint8_t** vertexBufferPointer)
{
    // TODO: Check what to include
    auto currentVertexBufferPointer = *vertexBufferPointer;

    // Positions
    ((float*)currentVertexBufferPointer)[0] = vertex->Position.X;
    ((float*)currentVertexBufferPointer)[1] = vertex->Position.Y;
    ((float*)currentVertexBufferPointer)[2] = vertex->Position.Z;

    currentVertexBufferPointer += sizeof(ElemToolsVector3);

    // Normals
    ((float*)currentVertexBufferPointer)[0] = vertex->Normal.X;
    ((float*)currentVertexBufferPointer)[1] = vertex->Normal.Y;
    ((float*)currentVertexBufferPointer)[2] = vertex->Normal.Z;

    currentVertexBufferPointer += sizeof(ElemToolsVector3);

    // Tangents
    ((float*)currentVertexBufferPointer)[0] = vertex->Tangent.X;
    ((float*)currentVertexBufferPointer)[1] = vertex->Tangent.Y;
    ((float*)currentVertexBufferPointer)[2] = vertex->Tangent.Z;
    ((float*)currentVertexBufferPointer)[3] = vertex->Tangent.W;

    currentVertexBufferPointer += sizeof(ElemToolsVector4);

    // Texture Coordinates
    ((float*)currentVertexBufferPointer)[0] = vertex->TextureCoordinates.X;
    ((float*)currentVertexBufferPointer)[1] = vertex->TextureCoordinates.Y;

    currentVertexBufferPointer += sizeof(ElemToolsVector2);
    
    *vertexBufferPointer = currentVertexBufferPointer;
}

ElemSceneMeshPrimitive ConstructObjMeshPrimitive(MemoryArena memoryArena, const fastObjMesh* objMesh, const ObjMeshPrimitiveInfo* meshPrimitiveInfo, const ElemLoadSceneOptions* options)
{
    auto stackMemoryArena = SystemGetStackMemoryArena();

    ElemSceneMeshPrimitive result =
    {
        .BoundingBox = 
        { 
            .MinPoint = { FLT_MAX, FLT_MAX, FLT_MAX }, 
            .MaxPoint = { FLT_MIN, FLT_MIN, FLT_MIN } 
        }
    };

    // TODO: Config of the vertex components to load
    auto positionSize = sizeof(float) * 3;
    auto normalSize = sizeof(float) * 3;
    auto tangentSize = sizeof(float) * 4;
    auto textureCoordinatesSize = sizeof(float) * 2;
    auto maxVertexSize = positionSize + normalSize + tangentSize + textureCoordinatesSize;

    // TODO: Temporary
    auto realVertexSize = maxVertexSize;

    // TODO: We can just have one loop again, it is good that we have separated the read and write
    auto vertexList = SystemPushArray<ObjVertex>(stackMemoryArena, meshPrimitiveInfo->VertexCount);

    for (uint32_t i = 0; i < meshPrimitiveInfo->VertexCount; i++)
    {
        // TODO: Here, we can directly write the vertex data to the buffer
        vertexList[i] = ReadObjVertex(objMesh->indices[meshPrimitiveInfo->VertexOffset + i], objMesh, options->CoordinateSystem, options->FlipVerticalTextureCoordinates);
        AddPointToBoundingBox(vertexList[i].Position, &result.BoundingBox);
    }

    auto indexCount = 0u;
    
    for (uint32_t i = 0; i < meshPrimitiveInfo->FaceCount; i++)
    {
        if (objMesh->face_vertices[meshPrimitiveInfo->FaceOffset + i] == 3)
        {
            indexCount += 3;
        }
        else
        {
            indexCount += 6;
        }
    }
    
    auto indexBufferData = SystemPushArray<uint32_t>(memoryArena, indexCount);
    auto currentIndex = 0u;
    auto currentObjIndex = 0u;

    for (uint32_t i = 0; i < meshPrimitiveInfo->FaceCount; i++)
    {
        auto faceVertexCount = objMesh->face_vertices[meshPrimitiveInfo->FaceOffset + i];

        if (faceVertexCount == 3)
        {
            indexBufferData[currentIndex] = currentObjIndex;
            indexBufferData[currentIndex + 1] = currentObjIndex + 1;
            indexBufferData[currentIndex + 2] = currentObjIndex + 2;

            if (options->CoordinateSystem == ElemSceneCoordinateSystem_LeftHanded)
            {
                auto tmp = indexBufferData[currentIndex + 1];
                indexBufferData[currentIndex + 1] = indexBufferData[currentIndex + 2];
                indexBufferData[currentIndex + 2] = tmp;
            }

            currentIndex += 3;
        }
        else
        {
            indexBufferData[currentIndex] = currentObjIndex;
            indexBufferData[currentIndex + 1] = currentObjIndex + 1;
            indexBufferData[currentIndex + 2] = currentObjIndex + 2;

            indexBufferData[currentIndex + 3] = currentObjIndex + 0;
            indexBufferData[currentIndex + 4] = currentObjIndex + 2;
            indexBufferData[currentIndex + 5] = currentObjIndex + 3;

            if (options->CoordinateSystem == ElemSceneCoordinateSystem_LeftHanded)
            {
                auto tmp = indexBufferData[currentIndex + 1];
                indexBufferData[currentIndex + 1] = indexBufferData[currentIndex + 2];
                indexBufferData[currentIndex + 2] = tmp;

                tmp = indexBufferData[currentIndex + 4];
                indexBufferData[currentIndex + 4] = indexBufferData[currentIndex + 5];
                indexBufferData[currentIndex + 5] = tmp;
            }

            currentIndex += 6;
        }

        currentObjIndex += faceVertexCount;
    }

    // TODO: To replace
    GenerateTangentVectorsParameters generateTangentParams =
    {
        .PositionPointer = &vertexList.Pointer[0].Position,
        .NormalPointer = &vertexList.Pointer[0].Normal,
        .TextureCoordinatesPointer = &vertexList.Pointer[0].TextureCoordinates,
        .TangentPointer = &vertexList.Pointer[0].Tangent,
        .VertexSize = sizeof(ObjVertex),
        .IndexData = indexBufferData
    };

    AssertIfFailed(GenerateTangentVectors(&generateTangentParams));

    auto vertexBufferData = SystemPushArray<uint8_t>(memoryArena, meshPrimitiveInfo->VertexCount * maxVertexSize);
    auto currentVertexBufferPointer = vertexBufferData.Pointer;

    // TODO: Remove that loop
    for (uint32_t i = 0; i < meshPrimitiveInfo->VertexCount; i++)
    {
        WriteObjVertex(&vertexList[i], &currentVertexBufferPointer);
    }

    result.VertexBuffer =
    {
        .Data = { .Items = vertexBufferData.Pointer, .Length = (uint32_t)vertexBufferData.Length },
        .VertexSize = (uint32_t)realVertexSize,
        .VertexCount = meshPrimitiveInfo->VertexCount
    };

    result.IndexBuffer = { .Items = indexBufferData.Pointer, .Length = (uint32_t)indexBufferData.Length }; 
    result.MaterialId = meshPrimitiveInfo->MaterialId;

    return result;
}

void ApplyObjMeshPrimitiveInverseTranslation(ElemToolsVector3 translation, ElemVertexBuffer* vertexBuffer)
{
    for (uint32_t i = 0; i < vertexBuffer->VertexCount; i++)
    {
        auto position = (ElemToolsVector3*)(vertexBuffer->Data.Items + i * vertexBuffer->VertexSize);
        *position = { position->X - translation.X, position->Y - translation.Y, position->Z - translation.Z };
    }
}

void ApplyObjBoundingBoxInverseTranslation(ElemToolsVector3 translation, ElemToolsBoundingBox* boundingBox)
{
    boundingBox->MinPoint = { boundingBox->MinPoint.X - translation.X, boundingBox->MinPoint.Y - translation.Y, boundingBox->MinPoint.Z - translation.Y };
    boundingBox->MaxPoint = { boundingBox->MaxPoint.X - translation.X, boundingBox->MaxPoint.Y - translation.Y, boundingBox->MaxPoint.Z - translation.Y };
}

ElemLoadSceneResult LoadObjSceneAndNodes(const fastObjMesh* objFileData, const ElemLoadSceneOptions* options)
{
    auto hasErrors = false;

    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto sceneLoaderMemoryArena = GetSceneLoaderMemoryArena();

    auto globalTransformMatrix = CreateSceneLoaderGlobalTransformMatrix(options);

    auto meshes = SystemPushArray<ElemSceneMesh>(sceneLoaderMemoryArena, objFileData->group_count);
    auto sceneNodes = SystemPushArray<ElemSceneNode>(sceneLoaderMemoryArena, objFileData->group_count);
    auto meshPrimitiveInfos = SystemPushArray<ObjMeshPrimitiveInfo>(stackMemoryArena, UINT16_MAX);

    for (uint32_t i = 0; i < objFileData->group_count; i++)
    {
        auto mesh = &meshes[i];
        auto sceneNode = &sceneNodes[i];
        auto groupData = &objFileData->groups[i];

        if (groupData->face_count == 0)
        {
            printf("Invalid object\n");
            continue;
        }

        auto vertexOffset = 0u;

        for (uint32_t j = 0; j < groupData->face_offset; j++) 
        {
            vertexOffset += objFileData->face_vertices[j];
        }

        auto currentMaterial = objFileData->face_materials[groupData->face_offset];
        auto meshPrimitiveCount = 0u;

        auto meshPrimitiveInfo = &meshPrimitiveInfos[meshPrimitiveCount++];
        *meshPrimitiveInfo = 
        { 
            .VertexOffset = vertexOffset, 
            .FaceOffset = groupData->face_offset,
            .MaterialId = (int32_t)currentMaterial
        };
      
        for (uint32_t j = 0; j < groupData->face_count; j++) 
        {
            auto faceMaterial = objFileData->face_materials[groupData->face_offset + j];
            auto faceVertexCount = objFileData->face_vertices[groupData->face_offset + j];

            if (faceVertexCount != 3 && faceVertexCount != 4)
            {
                return
                {
                    .Messages = ConstructErrorMessageSpan(sceneLoaderMemoryArena, "Obj mesh loader only support triangles or quads."),
                    .HasErrors = true
                };
            }

            if (faceMaterial != currentMaterial)
            {
                currentMaterial = faceMaterial;

                auto previousVertexOffset = meshPrimitiveInfo->VertexOffset;
                auto previousVertexCount = meshPrimitiveInfo->VertexCount;
                auto previousFaceOffset = meshPrimitiveInfo->FaceOffset;
                auto previousFaceCount = meshPrimitiveInfo->FaceCount;

                meshPrimitiveInfo = &meshPrimitiveInfos[meshPrimitiveCount++];
                *meshPrimitiveInfo = 
                { 
                    .VertexOffset = previousVertexOffset + previousVertexCount, 
                    .FaceOffset = previousFaceOffset + previousFaceCount,
                    .MaterialId = (int32_t)currentMaterial
                };
            }

            meshPrimitiveInfo->VertexCount += faceVertexCount;
            meshPrimitiveInfo->FaceCount++;
        }

        mesh->BoundingBox = 
        { 
            .MinPoint = { FLT_MAX, FLT_MAX, FLT_MAX }, 
            .MaxPoint = { -FLT_MAX, -FLT_MAX, -FLT_MAX } 
        }; 

        auto meshPrimitives = SystemPushArray<ElemSceneMeshPrimitive>(sceneLoaderMemoryArena, meshPrimitiveCount);

        for (uint32_t j = 0; j < meshPrimitiveCount; j++)
        {
            meshPrimitives[j] = ConstructObjMeshPrimitive(sceneLoaderMemoryArena, objFileData, &meshPrimitiveInfos[j], options);
            AddBoundingBoxToBoundingBox(&meshPrimitives[j].BoundingBox, &mesh->BoundingBox);
        }
            
        auto boundingBoxCenter = GetBoundingBoxCenter(&mesh->BoundingBox);
        ApplyObjBoundingBoxInverseTranslation(boundingBoxCenter, &mesh->BoundingBox);

        for (uint32_t j = 0; j < meshPrimitiveCount; j++)
        {
            auto meshPrimitive = &meshPrimitives[j];

            ApplyObjMeshPrimitiveInverseTranslation(boundingBoxCenter, &meshPrimitive->VertexBuffer);
            ApplyObjBoundingBoxInverseTranslation(boundingBoxCenter, &meshPrimitive->BoundingBox);
        }
        
        if (groupData->name)
        {
            mesh->Name = SystemDuplicateBuffer(sceneLoaderMemoryArena, ReadOnlySpan<char>(groupData->name)).Pointer;
        }
        else
        {
            mesh->Name = "Mesh";
        }

        mesh->MeshPrimitives = { .Items = meshPrimitives.Pointer, .Length = (uint32_t)meshPrimitives.Length };

        if (groupData->name)
        {
            sceneNode->Name = SystemDuplicateBuffer(sceneLoaderMemoryArena, ReadOnlySpan<char>(groupData->name)).Pointer;
        }
        else
        {
            sceneNode->Name = "Node";
        }

        sceneNode->NodeType = ElemSceneNodeType_Mesh;
        sceneNode->ReferenceIndex = i;

        auto nodeTransform = ElemToolsCreateTranslationMatrix({ boundingBoxCenter.X, boundingBoxCenter.Y, boundingBoxCenter.Z });

        nodeTransform = ElemToolsMulMatrix4x4(nodeTransform, globalTransformMatrix);
        DecomposeTransform(nodeTransform, &sceneNode->Scale, &sceneNode->Rotation, &sceneNode->Translation);
    }

    return 
    {
        .SceneFormat = ElemSceneFormat_Obj,
        .CoordinateSystem = options->CoordinateSystem,
        .Meshes = { .Items = meshes.Pointer, .Length = (uint32_t)meshes.Length },
        .Nodes = { .Items = sceneNodes.Pointer, .Length = (uint32_t)sceneNodes.Length },
        .HasErrors = hasErrors
    };
}

ElemSceneMaterialSpan LoadObjMaterials(const fastObjMesh* objFileData, const ElemLoadSceneOptions* options)
{
    auto sceneLoaderMemoryArena = GetSceneLoaderMemoryArena();
    auto materials = SystemPushArray<ElemSceneMaterial>(sceneLoaderMemoryArena, objFileData->material_count);

    for (uint32_t i = 0; i < objFileData->material_count; i++)
    {
        auto objMaterial = &objFileData->materials[i];
        auto material = &materials[i];

        if (objMaterial->name)
        {
            material->Name = SystemDuplicateBuffer(sceneLoaderMemoryArena, ReadOnlySpan<char>(objMaterial->name)).Pointer;
        }
        else
        {
            material->Name = "Material";
        }

        if (objMaterial->map_Kd > 0)
        {
            material->AlbedoTexturePath = SystemDuplicateBuffer(sceneLoaderMemoryArena, ReadOnlySpan<char>(objFileData->textures[objMaterial->map_Kd].path)).Pointer;
        }

        if (objMaterial->map_bump > 0)
        {
            material->NormalTexturePath = SystemDuplicateBuffer(sceneLoaderMemoryArena, ReadOnlySpan<char>(objFileData->textures[objMaterial->map_bump].path)).Pointer;
        }

        material->AlbedoFactor = {{ 1.0f, 1.0f, 1.0f, 1.0f }};
    }

    return { .Items = materials.Pointer, .Length = (uint32_t)materials.Length };
}

ElemLoadSceneResult LoadObjScene(const char* path, const ElemLoadSceneOptions* options)
{
    auto stackMemoryArena = SystemGetStackMemoryArena();

    auto callbacks = fastObjCallbacks
    {
        .file_open = FastObjFileOpen,
        .file_close = FastObjFileClose,
        .file_read = FastObjFileRead,
        .file_size = FastObjFileSize
    };

    auto objFileData = fast_obj_read_with_callbacks(path, &callbacks, &stackMemoryArena);

    auto result = LoadObjSceneAndNodes(objFileData, options);
    result.Materials = LoadObjMaterials(objFileData, options);

    return result;
}
