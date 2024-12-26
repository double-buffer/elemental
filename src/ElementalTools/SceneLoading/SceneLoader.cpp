#include "ElementalTools.h"
#include "SystemMemory.h"
#include "SceneLoaderObj.cpp"

// TODO: Do one for each thread
static MemoryArena SceneLoaderMemoryArena;

void InitSceneLoaderMemoryArena()
{
    if (SceneLoaderMemoryArena.Storage == nullptr)
    {
        SceneLoaderMemoryArena = SystemAllocateMemoryArena(512 * 1024 * 1024);
    }

    SystemClearMemoryArena(SceneLoaderMemoryArena);
}

MemoryArena GetSceneLoaderMemoryArena()
{
    return SceneLoaderMemoryArena;
}

ElemSceneFormat GetSceneFormatFromPath(const char* path)
{
    auto pathSpan = ReadOnlySpan<char>(path);
    auto stackMemoryArena = SystemGetStackMemoryArena();

    auto lastIndex = SystemLastIndexOf(path, '.');
    SystemAssert(lastIndex != -1);

    auto extension = SystemPushArray<char>(stackMemoryArena, pathSpan.Length - lastIndex);
    SystemCopyBuffer(extension, pathSpan.Slice(lastIndex + 1));

    if (SystemFindSubString(extension, "obj") != -1)
    {
        return ElemSceneFormat_Obj;
    }

    return ElemSceneFormat_Unknown;
}

ElemToolsAPI ElemLoadSceneResult ElemLoadScene(const char* path, const ElemLoadSceneOptions* options)
{
    InitSceneLoaderMemoryArena();

    ElemLoadSceneOptions loadSceneOptions = {};

    if (options)
    {
        loadSceneOptions = *options;
    }

    if (loadSceneOptions.Scaling == 0.0f)
    {
        loadSceneOptions.Scaling = 1.0f;
    }

    auto sceneFormat = GetSceneFormatFromPath(path);

    // TODO: Refactor that with array entries and function pointer
    switch (sceneFormat)
    {
        case ElemSceneFormat_Obj:
            return LoadObjScene(path, &loadSceneOptions);

        default:
            return
            {
                .Messages = ConstructErrorMessageSpan(GetSceneLoaderMemoryArena(), "Scene format is not supported."),
                .HasErrors = true
            };
    };
}
