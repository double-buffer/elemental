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

void ProcessObjVertex(fastObjIndex objVertex, const fastObjMesh* objMesh, ElemSceneCoordinateSystem coordinateSystem, uint8_t** vertexBufferPointer)
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

ElemVertexBuffer ConstructObjVertexBuffer(MemoryArena memoryArena, const fastObjMesh* objMesh, const ObjMeshPrimitiveInfo* meshPartInfo, const ElemLoadSceneOptions* options)
{
    SystemAssert(options);
    auto coordinateSystem = options->CoordinateSystem;
    
    // TODO: Config of the vertex components to load
    auto positionSize = sizeof(float) * 3;
    auto normalSize = sizeof(float) * 3;
    auto textureCoordinatesSize = sizeof(float) * 2;
    auto maxVertexSize = positionSize + normalSize + textureCoordinatesSize;

    // TODO: Temporary
    auto realVertexSize = maxVertexSize;

    auto vertexBuffer = SystemPushArray<uint8_t>(memoryArena, meshPartInfo->IndexCount * maxVertexSize);
    auto currentVertexBufferPointer = vertexBuffer.Pointer;

    for (uint32_t i = meshPartInfo->IndexOffset; i < meshPartInfo->IndexOffset + meshPartInfo->IndexCount; i += 3)
    {
        if (coordinateSystem == ElemSceneCoordinateSystem_LeftHanded)
        {
            ProcessObjVertex(objMesh->indices[i], objMesh, coordinateSystem, &currentVertexBufferPointer);
            ProcessObjVertex(objMesh->indices[i + 2], objMesh, coordinateSystem, &currentVertexBufferPointer);
            ProcessObjVertex(objMesh->indices[i + 1], objMesh, coordinateSystem, &currentVertexBufferPointer);
        }
        else
        {
            ProcessObjVertex(objMesh->indices[i], objMesh, coordinateSystem, &currentVertexBufferPointer);
            ProcessObjVertex(objMesh->indices[i + 1], objMesh, coordinateSystem, &currentVertexBufferPointer);
            ProcessObjVertex(objMesh->indices[i + 2], objMesh, coordinateSystem, &currentVertexBufferPointer);
        }
    }

    return 
    {
        .Data = { .Items = vertexBuffer.Pointer, .Length = (uint32_t)vertexBuffer.Length },
        .VertexSize = (uint32_t)realVertexSize,
        .VertexCount = meshPartInfo->IndexCount
    };
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
    auto meshPartInfos = SystemPushArray<ObjMeshPrimitiveInfo>(stackMemoryArena, UINT16_MAX);

    for (uint32_t i = 0; i < objFileData->object_count; i++)
    {
        auto mesh = &meshes[i];
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

        auto meshPartCount = 0u;

        auto meshPartInfo = &meshPartInfos[meshPartCount++];
        *meshPartInfo = { .IndexOffset = indexOffset };
      
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

                auto meshPartIndexOffset = indexOffset + meshPartInfo->IndexCount;

                meshPartInfo = &meshPartInfos[meshPartCount++];
                *meshPartInfo = { .IndexOffset = meshPartIndexOffset };
            }

            meshPartInfo->IndexCount += faceVertexCount;
        }

        auto meshParts = SystemPushArray<ElemSceneMeshPrimitive>(sceneLoaderMemoryArena, meshPartCount);

        for (uint32_t i = 0; i < meshPartCount; i++)
        {
            auto meshPart = &meshParts[i];
            auto meshPartInfo = &meshPartInfos[i];

            meshPart->VertexBuffer = ConstructObjVertexBuffer(sceneLoaderMemoryArena, objFileData, meshPartInfo, options);
        }

        mesh->MeshPrimitives = { .Items = meshParts.Pointer, .Length = (uint32_t)meshParts.Length };
    }

    return 
    {
        .SceneFormat = ElemSceneFormat_Obj,
        .CoordinateSystem = options->CoordinateSystem,
        .Meshes = { .Items = meshes.Pointer, .Length = (uint32_t)meshes.Length },
        .HasErrors = hasErrors
    };
}
