
#include "ElementalTools.h"
#include "SystemMemory.h"
#include "SystemFunctions.h"


// TODO: We need to track the offset

void* FastObjFileOpen(const char* path, void* userData)
{
    if (SystemFindSubString(path, "ObjData") != -1)
    {
        return nullptr;
    }

    return (void*)1;
}

void FastObjFileClose(void* file, void* userData)
{
}

size_t FastObjFileRead(void* file, void* destination, size_t bytes, void* userData)
{
    // TODO: Check File ID
    auto sourceDataSpan = (ElemToolsDataSpan*)userData;
    auto sourceSpan = ReadOnlySpan<uint8_t>(sourceDataSpan->Items, sourceDataSpan->Length);
    auto destinationSpan = Span<uint8_t>((uint8_t*)destination, bytes);
    
    printf("ReadObj: %d/%d\n", bytes, sourceDataSpan->Length);
    SystemCopyBuffer(destinationSpan, sourceSpan);

    return 0;
}

unsigned long FastObjFileSize(void* file, void* userData)
{
    // TODO: Check File ID
    auto dataSpan = (ElemToolsDataSpan*)userData;
    return dataSpan->Length;
}

ElemToolsAPI ElemLoadMeshResult ElemLoadMesh(ElemToolsDataSpan data, ElemMeshFormat meshFormat, const ElemLoadMeshOptions* options)
{
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

    auto mesh = fast_obj_read_with_callbacks("ObjFile", &callbacks, (void*)&data);

    return 
    {
        .HasErrors = hasErrors
    };
}
