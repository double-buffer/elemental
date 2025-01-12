#include "SceneLoader.h"
#include "ToolsUtils.h"
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
    uint32_t IndexOffset;
    uint32_t IndexCount; 
    int32_t MaterialId;
};

struct ObjVertex
{
    ElemToolsVector3 Position;
    ElemToolsVector3 Normal;
    ElemToolsVector4 Tangent;
    ElemToolsVector2 TextureCoordinates;
};

void* FastObjFileOpen(const char* path, void* userData)
{
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

    // TODO: For the map_Dist that isn't parsed by the lib, maybe we can replace it with bump here?
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

struct ObjTriangle
{
    uint32_t Vertices[3];
    // TODO: More data
};

// TODO: Generalize this function so that it can be used by other loaders
ElemToolsVector3 ComputeTriangleTangentVector(const ObjTriangle* triangle, ReadOnlySpan<ObjVertex> vertexList)
{
    // TODO: Refactor that
    ObjVertex triangleVertices[3];

    for (uint32_t j = 0; j < 3; j++)
    {
        triangleVertices[j] = vertexList[triangle->Vertices[j]];
    }

    // TODO: Put that in a function because we may use it of GLTF too if the tangents are missing
    // Compute tangents
    auto edge1 = triangleVertices[1].Position - triangleVertices[0].Position; 
    auto edge2 = triangleVertices[2].Position - triangleVertices[0].Position; 

    auto deltaUV1 = triangleVertices[1].TextureCoordinates - triangleVertices[0].TextureCoordinates; 
    auto deltaUV2 = triangleVertices[2].TextureCoordinates - triangleVertices[0].TextureCoordinates; 

    float f = 1.0f / (deltaUV1.X * deltaUV2.Y - deltaUV2.X * deltaUV1.Y);

    ElemToolsVector3 tangent = {};

    tangent.X = f * (deltaUV2.Y * edge1.X - deltaUV1.Y * edge2.X);
    tangent.Y = f * (deltaUV2.Y * edge1.Y - deltaUV1.Y * edge2.Y);
    tangent.Z = f * (deltaUV2.Y * edge1.Z - deltaUV1.Y * edge2.Z);

    return tangent;
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

    auto indexBufferData = SystemPushArray<uint32_t>(memoryArena, meshPrimitiveInfo->IndexCount);

    auto triangleCount = meshPrimitiveInfo->IndexCount / 3;
    auto triangleList = SystemPushArray<ObjTriangle>(stackMemoryArena, triangleCount);
    auto vertexList = SystemPushArray<ObjVertex>(stackMemoryArena, 3 * triangleCount);
    auto vertexCount = 0u;

    for (uint32_t i = 0; i < triangleCount; i++)
    {
        auto triangle = &triangleList[i];

        for (uint32_t j = 0; j < 3; j++)
        {
            auto vertex = ReadObjVertex(objMesh->indices[meshPrimitiveInfo->IndexOffset + i * 3 + j], objMesh, options->CoordinateSystem, options->FlipVerticalTextureCoordinates);

            // TODO: Check for duplicates
            auto vertexIndex = vertexCount++;
            vertexList[vertexIndex] = vertex;
            AddPointToBoundingBox(vertex.Position, &result.BoundingBox);

            triangle->Vertices[j] = vertexIndex;
            indexBufferData[i * 3 + j] = vertexIndex;
        }
        
        if (options->CoordinateSystem == ElemSceneCoordinateSystem_LeftHanded)
        {
            auto tmp = indexBufferData[i * 3 + 1];
            indexBufferData[i * 3 + 1] = indexBufferData[i * 3 + 2];
            indexBufferData[i * 3 + 2] = tmp;
        }

        auto triangleTangent = ComputeTriangleTangentVector(triangle, vertexList);

        for (uint32_t j = 0; j < 3; j++)
        {
            auto vertexIndex = triangle->Vertices[j];
            vertexList[vertexIndex].Tangent.XYZ = vertexList[vertexIndex].Tangent.XYZ + triangleTangent;
        }
    }
    
    auto vertexBufferData = SystemPushArray<uint8_t>(memoryArena, meshPrimitiveInfo->IndexCount * maxVertexSize);
    auto currentVertexBufferPointer = vertexBufferData.Pointer;

    for (uint32_t i = 0; i < vertexCount; i++)
    {
        auto normal = vertexList[i].Normal;
        auto tangent = ElemToolsNormalizeV3(vertexList[i].Tangent.XYZ);

        // TODO: Active that when we have unique vertices
        //tangent = ElemToolsNormalizeV3(tangent - normal * ElemToolsDotProductV3(normal, tangent));

        // TODO: Process vertex data
        vertexList[i].Tangent.XYZ = tangent;
        vertexList[i].Tangent.W = 1.0f;
        //float handedness = (glm::dot(glm::cross(normal, tangent), B) < 0.0f) ? -1.0f : 1.0f;

        WriteObjVertex(&vertexList[i], &currentVertexBufferPointer);
    }

    result.VertexBuffer =
    {
        .Data = { .Items = vertexBufferData.Pointer, .Length = (uint32_t)vertexBufferData.Length },
        .VertexSize = (uint32_t)realVertexSize,
        .VertexCount = meshPrimitiveInfo->IndexCount
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

    auto meshes = SystemPushArray<ElemSceneMesh>(sceneLoaderMemoryArena, objFileData->object_count);
    auto sceneNodes = SystemPushArray<ElemSceneNode>(sceneLoaderMemoryArena, objFileData->object_count);
    auto meshPrimitiveInfos = SystemPushArray<ObjMeshPrimitiveInfo>(stackMemoryArena, UINT16_MAX);

    for (uint32_t i = 0; i < objFileData->object_count; i++)
    {
        auto mesh = &meshes[i];
        auto sceneNode = &sceneNodes[i];
        auto objectData = &objFileData->objects[i];

        if (objectData->face_count == 0)
        {
            printf("Invalid object\n");
            continue;
        }

        auto currentFaceStart = objectData->face_offset;
        auto currentFaceEnd = objectData->face_offset + objectData->face_count; 

        auto indexOffset = 0u;

        for (uint32_t j = 0; j < currentFaceStart; j++) 
        {
            indexOffset += objFileData->face_vertices[j];
        }

        auto currentMaterial = objFileData->face_materials[currentFaceStart];
        auto meshPrimitiveCount = 0u;

        auto meshPrimitiveInfo = &meshPrimitiveInfos[meshPrimitiveCount++];
        *meshPrimitiveInfo = 
        { 
            .IndexOffset = indexOffset, 
            .MaterialId = (int32_t)currentMaterial
        };
      
        for (uint32_t j = currentFaceStart; j < currentFaceEnd; j++) 
        {
            auto faceMaterial = objFileData->face_materials[j];
            int faceVertexCount = objFileData->face_vertices[j];

            if (faceVertexCount != 3)
            {
                return
                {
                    .Messages = ConstructErrorMessageSpan(sceneLoaderMemoryArena, "Obj mesh loader only support triangles."),
                    .HasErrors = true
                };
            }

            if (faceMaterial != currentMaterial)
            {
                currentMaterial = faceMaterial;
                printf("Material: %d\n", currentMaterial);

                auto meshPrimitiveIndexOffset = indexOffset + meshPrimitiveInfo->IndexCount;

                meshPrimitiveInfo = &meshPrimitiveInfos[meshPrimitiveCount++];
                *meshPrimitiveInfo = 
                { 
                    .IndexOffset = meshPrimitiveIndexOffset, 
                    .MaterialId = (int32_t)currentMaterial
                };
            }

            meshPrimitiveInfo->IndexCount += faceVertexCount;
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
        
        if (objectData->name)
        {
            mesh->Name = SystemDuplicateBuffer(sceneLoaderMemoryArena, ReadOnlySpan<char>(objectData->name)).Pointer;
        }
        else
        {
            mesh->Name = "Mesh";
        }

        mesh->MeshPrimitives = { .Items = meshPrimitives.Pointer, .Length = (uint32_t)meshPrimitives.Length };

        if (objectData->name)
        {
            sceneNode->Name = SystemDuplicateBuffer(sceneLoaderMemoryArena, ReadOnlySpan<char>(objectData->name)).Pointer;
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

        printf("material: %s\n", objFileData->materials[i].name);
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
