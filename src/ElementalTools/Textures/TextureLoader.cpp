#include "TextureLoader.h"
#include "TextureLoaderStb.cpp"

// TODO: Do one for each thread
static MemoryArena TextureLoaderMemoryArena;

void InitTextureLoaderMemoryArena()
{
    if (TextureLoaderMemoryArena.Storage == nullptr)
    {
        TextureLoaderMemoryArena = SystemAllocateMemoryArena(512 * 1024 * 1024);
    }

    SystemClearMemoryArena(TextureLoaderMemoryArena);
}

MemoryArena GetTextureLoaderMemoryArena()
{
    return TextureLoaderMemoryArena;
}

ElemLoadTextureFileFormat GetLoadTextureFileFormatFromPath(const char* path)
{
    auto pathSpan = ReadOnlySpan<char>(path);
    auto stackMemoryArena = SystemGetStackMemoryArena();

    auto lastIndex = SystemLastIndexOf(path, '.');
    SystemAssert(lastIndex != -1);

    auto extension = SystemPushArray<char>(stackMemoryArena, pathSpan.Length - lastIndex);
    SystemCopyBuffer(extension, pathSpan.Slice(lastIndex + 1));

    if (SystemFindSubString(extension, "tga") != -1)
    {
        return ElemLoadTextureFileFormat_Tga;
    }

    return ElemLoadTextureFileFormat_Unknown;
}

ElemToolsAPI ElemLoadTextureResult ElemLoadTexture(const char* path, const ElemLoadTextureOptions* options)
{
    InitTextureLoaderMemoryArena();

    ElemLoadTextureOptions loadTextureOptions = {};

    if (options)
    {
        loadTextureOptions = *options;
    }

    auto textureFileFormat = GetLoadTextureFileFormatFromPath(path);

    // TODO: Refactor that with array entries and function pointer
    switch (textureFileFormat)
    {
        case ElemLoadTextureFileFormat_Tga:
            return LoadStbTexture(path, textureFileFormat, &loadTextureOptions);

        default:
            return
            {
                .Messages = ConstructErrorMessageSpan(GetTextureLoaderMemoryArena(), "Texture format is not supported."),
                .HasErrors = true
            };
    };
}
