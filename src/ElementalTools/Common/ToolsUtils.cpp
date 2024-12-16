#include "ToolsUtils.h"
#include "ElementalTools.h"
#include "SystemFunctions.h"
#include "SystemMemory.h"

// TODO: Do one for each threads
static MemoryArena FileIOMemoryArena;

ElemToolsDataSpan DefaultFileHandler(const char* path)
{
    if (SystemFileExists(path))
    {
        auto data = SystemFileReadBytes(FileIOMemoryArena, path);
        return { .Items = data.Pointer, .Length = (uint32_t)data.Length };
    }

    return {};
}

static ElemToolsLoadFileHandlerPtr loadFileHandlerPtr = DefaultFileHandler;

ElemToolsAPI void ElemToolsConfigureFileIO(ElemToolsLoadFileHandlerPtr loadFileHandler)
{
    SystemAssert(loadFileHandler);
    loadFileHandlerPtr = loadFileHandler;
}

void InitStorageMemoryArena()
{
    if (FileIOMemoryArena.Storage == nullptr)
    {
        FileIOMemoryArena = SystemAllocateMemoryArena();
    }
}

ReadOnlySpan<uint8_t> LoadFileData(const char* path)
{
    InitStorageMemoryArena();

    auto data = loadFileHandlerPtr(path);
    return ReadOnlySpan<uint8_t>(data.Items, data.Length);
}

void ResetLoadFileDataMemory()
{
    SystemClearMemoryArena(FileIOMemoryArena);
}
