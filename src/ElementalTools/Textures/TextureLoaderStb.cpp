#include "TextureLoader.h"
#include "ToolsUtils.h"
#include "ElementalTools.h"
#include "SystemMemory.h"
#include "SystemFunctions.h"

ElemLoadTextureResult LoadStbTexture(const char* path, ElemLoadTextureFileFormat fileFormat, const ElemLoadTextureOptions* options)
{
    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto textureLoaderMemoryArena = GetTextureLoaderMemoryArena();

    auto messages = SystemPushArray<ElemToolsMessage>(textureLoaderMemoryArena, 1024);
    auto messageCount = 0u;
    auto hasErrors = false;

    auto objFileData = LoadFileData(path); 

    if (objFileData.Length == 0)
    {
        return { .Messages = ConstructErrorMessageSpan(textureLoaderMemoryArena, "Error while reading texture file."), .HasErrors = true };
    }

    int32_t width, height, channels;
    auto stbImageData = stbi_load_from_memory(objFileData.Pointer, objFileData.Length, &width, &height, &channels, STBI_rgb_alpha);

    if (!stbImageData)
    {
        return { .Messages = ConstructErrorMessageSpan(textureLoaderMemoryArena, "Error while loading texture file."), .HasErrors = true };
    }

    auto imageDataLength = width * height * STBI_rgb_alpha;
    auto imageData = SystemDuplicateBuffer(textureLoaderMemoryArena, ReadOnlySpan<uint8_t>(stbImageData, imageDataLength));

    auto mipData = SystemPushStruct<ElemTextureMipData>(textureLoaderMemoryArena);
    mipData->Width = width;
    mipData->Height = height;
    mipData->Data = { .Items = imageData.Pointer, .Length = (uint32_t)imageData.Length },

    stbi_image_free(stbImageData);
    ResetLoadFileDataMemory();

    return 
    {
        .FileFormat = fileFormat,
        .Format = ElemToolsGraphicsFormat_R8G8B8A8_SRGB,
        .Width = (uint32_t)width,
        .Height = (uint32_t)height,
        .MipData = { .Items = mipData, .Length = 1 },
        .Messages = { .Items = messages.Pointer, .Length = messageCount },
        .HasErrors = hasErrors
    };
}
