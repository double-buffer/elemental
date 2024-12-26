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

void AddPointToBoundingBox(ElemToolsVector3 point, ElemToolsBoundingBox* boundingBox)
{
    float minX = (point.X < boundingBox->MinPoint.X) ? point.X : boundingBox->MinPoint.X;
    float minY = (point.Y < boundingBox->MinPoint.Y) ? point.Y : boundingBox->MinPoint.Y;
    float minZ = (point.Z < boundingBox->MinPoint.Z) ? point.Z : boundingBox->MinPoint.Z;

    float maxX = (point.X > boundingBox->MaxPoint.X) ? point.X : boundingBox->MaxPoint.X;
    float maxY = (point.Y > boundingBox->MaxPoint.Y) ? point.Y : boundingBox->MaxPoint.Y;
    float maxZ = (point.Z > boundingBox->MaxPoint.Z) ? point.Z : boundingBox->MaxPoint.Z;

    boundingBox->MinPoint = { minX, minY, minZ };
    boundingBox->MaxPoint = { maxX, maxY, maxZ };
}

void AddBoundingBoxToBoundingBox(const ElemToolsBoundingBox* additional, ElemToolsBoundingBox* boundingBox)
{
    float minX = SystemMin(boundingBox->MinPoint.X, additional->MinPoint.X);
    float minY = SystemMin(boundingBox->MinPoint.Y, additional->MinPoint.Y);
    float minZ = SystemMin(boundingBox->MinPoint.Z, additional->MinPoint.Z);

    float maxX = SystemMax(boundingBox->MaxPoint.X, additional->MaxPoint.X);
    float maxY = SystemMax(boundingBox->MaxPoint.Y, additional->MaxPoint.Y);
    float maxZ = SystemMax(boundingBox->MaxPoint.Z, additional->MaxPoint.Z);

    boundingBox->MinPoint = { minX, minY, minZ };
    boundingBox->MaxPoint = { maxX, maxY, maxZ };
}

ElemToolsVector3 GetBoundingBoxCenter(const ElemToolsBoundingBox* boundingBox)
{
    return
    {
        .X = (boundingBox->MinPoint.X + boundingBox->MaxPoint.X) * 0.5f,
        .Y = (boundingBox->MinPoint.Y + boundingBox->MaxPoint.Y) * 0.5f,
        .Z = (boundingBox->MinPoint.Z + boundingBox->MaxPoint.Z) * 0.5f
    };
}

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

void ProcessObjVertex(fastObjIndex objVertex, const fastObjMesh* objMesh, ElemSceneCoordinateSystem coordinateSystem, float scaling, uint8_t** vertexBufferPointer, ElemToolsBoundingBox* boundingBox)
{
    // TODO: Check what to include
    auto currentVertexBufferPointer = *vertexBufferPointer;

    if (objVertex.p)
    {
        for (uint32_t j = 0; j < 3; j++)
        {
            ((float*)currentVertexBufferPointer)[j] = objMesh->positions[3 * objVertex.p + j] * scaling;
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

ElemVertexBuffer ConstructObjVertexBuffer(MemoryArena memoryArena, const fastObjMesh* objMesh, ObjMeshPrimitiveInfo* meshPrimitiveInfo, const ElemLoadSceneOptions* options)
{
    SystemAssert(options);
    auto coordinateSystem = options->CoordinateSystem;
    auto scaling = options->Scaling;
    
    // TODO: Config of the vertex components to load
    auto positionSize = sizeof(float) * 3;
    auto normalSize = sizeof(float) * 3;
    auto textureCoordinatesSize = sizeof(float) * 2;
    auto maxVertexSize = positionSize + normalSize + textureCoordinatesSize;

    // TODO: Temporary
    auto realVertexSize = maxVertexSize;

    auto vertexBuffer = SystemPushArray<uint8_t>(memoryArena, meshPrimitiveInfo->IndexCount * maxVertexSize);
    auto currentVertexBufferPointer = vertexBuffer.Pointer;

    for (uint32_t i = meshPrimitiveInfo->IndexOffset; i < meshPrimitiveInfo->IndexOffset + meshPrimitiveInfo->IndexCount; i += 3)
    {
        if (coordinateSystem == ElemSceneCoordinateSystem_LeftHanded)
        {
            ProcessObjVertex(objMesh->indices[i], objMesh, coordinateSystem, scaling, &currentVertexBufferPointer, &meshPrimitiveInfo->BoundingBox);
            ProcessObjVertex(objMesh->indices[i + 2], objMesh, coordinateSystem, scaling, &currentVertexBufferPointer, &meshPrimitiveInfo->BoundingBox);
            ProcessObjVertex(objMesh->indices[i + 1], objMesh, coordinateSystem, scaling, &currentVertexBufferPointer, &meshPrimitiveInfo->BoundingBox);
        }
        else
        {
            ProcessObjVertex(objMesh->indices[i], objMesh, coordinateSystem, scaling, &currentVertexBufferPointer, &meshPrimitiveInfo->BoundingBox);
            ProcessObjVertex(objMesh->indices[i + 1], objMesh, coordinateSystem, scaling, &currentVertexBufferPointer, &meshPrimitiveInfo->BoundingBox);
            ProcessObjVertex(objMesh->indices[i + 2], objMesh, coordinateSystem, scaling, &currentVertexBufferPointer, &meshPrimitiveInfo->BoundingBox);
        }
    }

    return 
    {
        .Data = { .Items = vertexBuffer.Pointer, .Length = (uint32_t)vertexBuffer.Length },
        .VertexSize = (uint32_t)realVertexSize,
        .VertexCount = meshPrimitiveInfo->IndexCount
    };
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

        printf("Object/Mesh: %s\n", objectData->name);

        auto currentFaceStart = objectData->face_offset;
        auto currentFaceEnd = objectData->face_offset + objectData->face_count; 

        auto indexOffset = 0u;

        for (uint32_t j = 0; j < currentFaceStart; j++) 
        {
            indexOffset += objFileData->face_vertices[j];
        }

        auto currentMaterial = objFileData->face_materials[currentFaceStart];
        printf("   MeshPrimitive (Material: %d)\n", currentMaterial);

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

                printf("   MeshPrimitive (Material: %d)\n", currentMaterial);

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

            meshPrimitive->VertexBuffer = ConstructObjVertexBuffer(sceneLoaderMemoryArena, objFileData, meshPrimitiveInfo, options);
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

        mesh->MeshPrimitives = { .Items = meshPrimitives.Pointer, .Length = (uint32_t)meshPrimitives.Length };

        sceneNode->Name = SystemDuplicateBuffer(sceneLoaderMemoryArena, ReadOnlySpan<char>(objectData->name)).Pointer;
        sceneNode->NodeType = ElemSceneNodeType_Mesh;
        sceneNode->ReferenceIndex = i;
        sceneNode->Rotation = {};
        sceneNode->Translation = boundingBoxCenter;
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
