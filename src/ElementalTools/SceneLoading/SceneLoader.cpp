#include "ToolsUtils.h"
#include "ElementalTools.h"
#include "SystemMemory.h"
#include "SystemFunctions.h"

// TODO: Do one for each thread
static MemoryArena SceneLoaderMemoryArena;

struct ObjLoaderFileData
{
    ReadOnlySpan<uint8_t> FileData;
    uint32_t CurrentOffset;
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

ElemVertexBuffer ConstructObjVertexBuffer(MemoryArena memoryArena, const fastObjMesh* objMesh, const ElemLoadSceneOptions* options)
{
    for (uint32_t i = 0; i < objMesh->object_count; i++)
    {
        auto object = objMesh->objects[i];
        printf("Object (Node): %s\n", object.name);

        /*
        for (uint32_t j = 0; j < object.face_count; j++)
        {
            auto face = objMesh->face_materials
            auto group = objMesh->face_materials[j];
            printf("Group: %s\n", group.name);
        }*/
    }

    // TODO: Handle sub objects
    ElemSceneCoordinateSystem coordinateSystem = ElemSceneCoordinateSystem_LeftHanded;

    if (options != nullptr)
    {
        coordinateSystem = options->SceneCoordinateSystem;
    }
    
    auto positionSize = sizeof(float) * 3;
    auto normalSize = sizeof(float) * 3;
    auto textureCoordinatesSize = sizeof(float) * 2;
    auto maxVertexSize = positionSize + normalSize + textureCoordinatesSize;

    // TODO: Temporary
    auto realVertexSize = maxVertexSize;

    auto vertexBuffer = SystemPushArray<uint8_t>(memoryArena, objMesh->index_count * maxVertexSize);
    auto currentVertexBufferPointer = vertexBuffer.Pointer;

    SystemAssert((objMesh->index_count % 3) == 0);

    for (uint32_t i = 0; i < objMesh->index_count; i += 3)
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
        .Data = { .Items = vertexBuffer.Pointer, .Length = objMesh->index_count * (uint32_t)realVertexSize },
        .VertexSize = (uint32_t)realVertexSize
    };
}

void InitSceneLoaderMemoryArena()
{
    if (SceneLoaderMemoryArena.Storage == nullptr)
    {
        SceneLoaderMemoryArena = SystemAllocateMemoryArena(512 * 1024 * 1024);
    }

    SystemClearMemoryArena(SceneLoaderMemoryArena);
}

// TODO: Split object loaders into multiple files
ElemToolsAPI ElemLoadSceneResult ElemLoadScene(const char* path, const ElemLoadSceneOptions* options)
{
    InitSceneLoaderMemoryArena();

    // TODO: To output the results, we should use a separate memory arena for each modules

    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto hasErrors = false;

    auto callbacks = fastObjCallbacks
    {
        .file_open = FastObjFileOpen,
        .file_close = FastObjFileClose,
        .file_read = FastObjFileRead,
        .file_size = FastObjFileSize
    };

    auto mesh = fast_obj_read_with_callbacks(path, &callbacks, &stackMemoryArena);

    // TODO: Support instances and meshes
    auto vertexBuffer = ConstructObjVertexBuffer(SceneLoaderMemoryArena, mesh, options);

    return 
    {
        .VertexBuffer = vertexBuffer,
        .VertexCount = mesh->index_count,
        .HasErrors = hasErrors
    };
}
