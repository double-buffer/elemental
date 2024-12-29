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
    ElemToolsBoundingBox BoundingBox;
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

// TODO: Remove that function and include it in construct vertex buffer
void ProcessObjVertex(fastObjIndex objVertex, const fastObjMesh* objMesh, ElemSceneCoordinateSystem coordinateSystem, uint8_t** vertexBufferPointer, ElemToolsBoundingBox* boundingBox)
{
    // TODO: Check what to include
    auto currentVertexBufferPointer = *vertexBufferPointer;

    if (objVertex.p)
    {
        for (uint32_t j = 0; j < 3; j++)
        {
            ((float*)currentVertexBufferPointer)[j] = objMesh->positions[3 * objVertex.p + j];
        }

        if (coordinateSystem == ElemSceneCoordinateSystem_LeftHanded)
        {
            ((float*)currentVertexBufferPointer)[2] = -((float*)currentVertexBufferPointer)[2];
        }

        AddPointToBoundingBox({ ((float*)currentVertexBufferPointer)[0], ((float*)currentVertexBufferPointer)[1], ((float*)currentVertexBufferPointer)[2]}, boundingBox);
        currentVertexBufferPointer += 3 * sizeof(float);
    }

    if (objVertex.n)
    {
        for (uint32_t j = 0; j < 3; j++)
        {
            ((float*)currentVertexBufferPointer)[j] = objMesh->normals[3 * objVertex.n + j];
        }

        if (coordinateSystem == ElemSceneCoordinateSystem_LeftHanded)
        {
            ((float*)currentVertexBufferPointer)[2] = -((float*)currentVertexBufferPointer)[2];
        }

        currentVertexBufferPointer += 3 * sizeof(float);
    }

    if (objVertex.t)
    {
        for (uint32_t j = 0; j < 2; j++)
        {
            ((float*)currentVertexBufferPointer)[j] = objMesh->texcoords[2 * objVertex.t + j];
        }
    }

    // TODO: Force the texture coordinates for now
    currentVertexBufferPointer += 2 * sizeof(float);
    *vertexBufferPointer = currentVertexBufferPointer;
}

ElemVertexBuffer ConstructObjVertexBuffer(MemoryArena memoryArena, ElemSceneCoordinateSystem coordinateSystem, const fastObjMesh* objMesh, ObjMeshPrimitiveInfo* meshPrimitiveInfo)
{
    // TODO: Config of the vertex components to load
    auto positionSize = sizeof(float) * 3;
    auto normalSize = sizeof(float) * 3;
    auto textureCoordinatesSize = sizeof(float) * 2;
    auto maxVertexSize = positionSize + normalSize + textureCoordinatesSize;

    // TODO: Temporary
    auto realVertexSize = maxVertexSize;

    auto vertexBuffer = SystemPushArray<uint8_t>(memoryArena, meshPrimitiveInfo->IndexCount * maxVertexSize);
    auto currentVertexBufferPointer = vertexBuffer.Pointer;

    for (uint32_t i = meshPrimitiveInfo->IndexOffset; i < meshPrimitiveInfo->IndexOffset + meshPrimitiveInfo->IndexCount; i++)
    {
        ProcessObjVertex(objMesh->indices[i], objMesh, coordinateSystem, &currentVertexBufferPointer, &meshPrimitiveInfo->BoundingBox);
    }

    return 
    {
        .Data = { .Items = vertexBuffer.Pointer, .Length = (uint32_t)vertexBuffer.Length },
        .VertexSize = (uint32_t)realVertexSize,
        .VertexCount = meshPrimitiveInfo->IndexCount
    };
}

ElemUInt32Span ConstructObjIndexBuffer(MemoryArena memoryArena, ElemSceneCoordinateSystem coordinateSystem, const fastObjMesh* objMesh, ObjMeshPrimitiveInfo* meshPrimitiveInfo)
{
    auto indexBuffer = SystemPushArray<uint32_t>(memoryArena, meshPrimitiveInfo->IndexCount);

    for (uint32_t i = 0; i < meshPrimitiveInfo->IndexCount; i += 3)
    {
        indexBuffer[i] = i;

        if (coordinateSystem == ElemSceneCoordinateSystem_LeftHanded)
        {
            indexBuffer[i + 1] = i + 2;
            indexBuffer[i + 2] = i + 1;
        }
        else
        {
            indexBuffer[i + 1] = i + 1;
            indexBuffer[i + 2] = i + 2;
        }
    }

    return { .Items = indexBuffer.Pointer, .Length = (uint32_t)indexBuffer.Length }; 
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

ElemLoadSceneResult LoadObjScene(const char* path, const ElemLoadSceneOptions* options)
{
    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto hasErrors = false;

    auto globalTransformMatrix = CreateSceneLoaderGlobalTransformMatrix(options);

    auto callbacks = fastObjCallbacks
    {
        .file_open = FastObjFileOpen,
        .file_close = FastObjFileClose,
        .file_read = FastObjFileRead,
        .file_size = FastObjFileSize
    };

    auto objFileData = fast_obj_read_with_callbacks(path, &callbacks, &stackMemoryArena);
    auto sceneLoaderMemoryArena = GetSceneLoaderMemoryArena();
    
    auto meshes = SystemPushArray<ElemSceneMesh>(sceneLoaderMemoryArena, objFileData->object_count);
    auto sceneNodes = SystemPushArray<ElemSceneNode>(sceneLoaderMemoryArena, objFileData->object_count);
    auto meshPrimitiveInfos = SystemPushArray<ObjMeshPrimitiveInfo>(stackMemoryArena, UINT16_MAX);

    for (uint32_t i = 0; i < objFileData->material_count; i++)
    {
        printf("Material: %s\n", objFileData->materials[i].name);
    }

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
            .BoundingBox = 
            { 
                .MinPoint = { FLT_MAX, FLT_MAX, FLT_MAX }, 
                .MaxPoint = { -FLT_MAX, -FLT_MAX, -FLT_MAX } 
            } 
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

                auto meshPrimitiveIndexOffset = indexOffset + meshPrimitiveInfo->IndexCount;

                meshPrimitiveInfo = &meshPrimitiveInfos[meshPrimitiveCount++];
                *meshPrimitiveInfo = 
                { 
                    .IndexOffset = meshPrimitiveIndexOffset, 
                    .BoundingBox = 
                    { 
                        .MinPoint = { FLT_MAX, FLT_MAX, FLT_MAX }, 
                        .MaxPoint = { FLT_MIN, FLT_MIN, FLT_MIN } 
                    } 
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
            auto meshPrimitive = &meshPrimitives[j];
            auto meshPrimitiveInfo = &meshPrimitiveInfos[j];

            meshPrimitive->VertexBuffer = ConstructObjVertexBuffer(sceneLoaderMemoryArena, options->CoordinateSystem, objFileData, meshPrimitiveInfo);
            meshPrimitive->IndexBuffer = ConstructObjIndexBuffer(sceneLoaderMemoryArena, options->CoordinateSystem, objFileData, meshPrimitiveInfo);
            meshPrimitive->BoundingBox = meshPrimitiveInfo->BoundingBox;

            AddBoundingBoxToBoundingBox(&meshPrimitive->BoundingBox, &mesh->BoundingBox);
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

    ResetLoadFileDataMemory();

    return 
    {
        .SceneFormat = ElemSceneFormat_Obj,
        .CoordinateSystem = options->CoordinateSystem,
        .Meshes = { .Items = meshes.Pointer, .Length = (uint32_t)meshes.Length },
        .Nodes = { .Items = sceneNodes.Pointer, .Length = (uint32_t)sceneNodes.Length },
        .HasErrors = hasErrors
    };
}
