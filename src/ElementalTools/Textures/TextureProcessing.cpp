#include "ElementalTools.h"
#include "SystemMemory.h"
#include "SystemFunctions.h"

// TODO: Do one for each thread
static MemoryArena generateMipDataMemoryArena;

void InitgenerateMipDataMemoryArena()
{
    if (generateMipDataMemoryArena.Storage == nullptr)
    {
        generateMipDataMemoryArena = SystemAllocateMemoryArena(512 * 1024 * 1024);
    }

    SystemClearMemoryArena(generateMipDataMemoryArena);
}

ElemToolsAPI ElemGenerateTextureMipDataResult ElemGenerateTextureMipData(ElemToolsGraphicsFormat format, ElemTextureMipData baseMip, const ElemGenerateTextureMipDataOptions* options)
{
    InitgenerateMipDataMemoryArena();

    auto messages = SystemPushArray<ElemToolsMessage>(generateMipDataMemoryArena, 1024);
    auto messageCount = 0u;
    auto hasErrors = false;

    ElemGenerateTextureMipDataOptions generateMipDataOptions = {};

    if (options)
    {
        generateMipDataOptions = *options;
    }

    auto pixelSize = 4; // TODO: Check format first
    auto maxDimension = SystemMax(baseMip.Width, baseMip.Height);
    auto mipCount = (uint32_t)floorf(log2f((float)maxDimension)) + 1;

    auto mipData = SystemPushArray<ElemTextureMipData>(generateMipDataMemoryArena, mipCount);
    auto baseMipCopy = SystemDuplicateBuffer(generateMipDataMemoryArena, ReadOnlySpan<uint8_t>(baseMip.Data.Items, baseMip.Data.Length));

    mipData[0] = 
    {
        .Width = baseMip.Width,
        .Height = baseMip.Height,
        .Data = { .Items = baseMipCopy.Pointer, .Length = (uint32_t)baseMipCopy.Length }
    };

    for (uint32_t i = 1; i < mipCount; i++)
    {
        auto mipLevelData = &mipData[i];

        mipLevelData->Width = SystemMax(1u, baseMip.Width >> i);
        mipLevelData->Height = SystemMax(1u, baseMip.Height >> i);

        auto mipLevelPixels = SystemPushArray<uint8_t>(generateMipDataMemoryArena, mipLevelData->Width * mipLevelData->Height * pixelSize);

        stbir_resize_uint8_srgb(baseMip.Data.Items, baseMip.Width, baseMip.Height, 0,
                                mipLevelPixels.Pointer, mipLevelData->Width, mipLevelData->Height, 0,
                                STBIR_RGBA);

        mipLevelData->Data = { .Items = mipLevelPixels.Pointer, .Length = (uint32_t)mipLevelPixels.Length };
    }

    return
    {
        .MipData = { .Items = mipData.Pointer, .Length = (uint32_t)mipData.Length },
        .Messages = { .Items = messages.Pointer, .Length = messageCount },
        .HasErrors = hasErrors
    };
}
