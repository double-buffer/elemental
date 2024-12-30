#include "ElementalTools.h"
#include "ToolsUtils.h"
#include "SystemMemory.h"
#include "SystemFunctions.h"

#define TEXTURE_BC_BLOCK_SIZE_IN_BYTES 16

// TODO: Do one for each thread
static MemoryArena generateMipDataMemoryArena;
static MemoryArena compressMipDataMemoryArena;

void InitGenerateMipDataMemoryArena()
{
    if (generateMipDataMemoryArena.Storage == nullptr)
    {
        generateMipDataMemoryArena = SystemAllocateMemoryArena(512 * 1024 * 1024);
    }

    SystemClearMemoryArena(generateMipDataMemoryArena);
}

void InitCompressMipDataMemoryArena()
{
    if (compressMipDataMemoryArena.Storage == nullptr)
    {
        compressMipDataMemoryArena = SystemAllocateMemoryArena(512 * 1024 * 1024);
    }

    SystemClearMemoryArena(compressMipDataMemoryArena);
}

ElemToolsAPI ElemGenerateTextureMipDataResult ElemGenerateTextureMipData(ElemToolsGraphicsFormat format, const ElemTextureMipData* baseMip, const ElemGenerateTextureMipDataOptions* options)
{
    InitGenerateMipDataMemoryArena();

    auto messages = SystemPushArray<ElemToolsMessage>(generateMipDataMemoryArena, 1024);
    auto messageCount = 0u;
    auto hasErrors = false;

    ElemGenerateTextureMipDataOptions generateMipDataOptions = {};

    if (options)
    {
        generateMipDataOptions = *options;
    }

    auto pixelSize = 4; // TODO: Check format first
    auto maxDimension = SystemMax(baseMip->Width, baseMip->Height);
    auto mipCount = (uint32_t)floorf(log2f((float)maxDimension)) + 1;

    auto mipData = SystemPushArray<ElemTextureMipData>(generateMipDataMemoryArena, mipCount);
    auto baseMipCopy = SystemDuplicateBuffer(generateMipDataMemoryArena, ReadOnlySpan<uint8_t>(baseMip->Data.Items, baseMip->Data.Length));

    mipData[0] = 
    {
        .Width = baseMip->Width,
        .Height = baseMip->Height,
        .Data = { .Items = baseMipCopy.Pointer, .Length = (uint32_t)baseMipCopy.Length }
    };

    for (uint32_t i = 1; i < mipCount; i++)
    {
        auto mipLevelData = &mipData[i];

        mipLevelData->Width = SystemMax(1u, baseMip->Width >> i);
        mipLevelData->Height = SystemMax(1u, baseMip->Height >> i);

        auto mipLevelPixels = SystemPushArray<uint8_t>(generateMipDataMemoryArena, mipLevelData->Width * mipLevelData->Height * pixelSize);

        stbir_resize_uint8_srgb(baseMip->Data.Items, baseMip->Width, baseMip->Height, 0,
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

ElemToolsAPI ElemCompressTextureMipDataResult ElemCompressTextureMipData(ElemToolsGraphicsFormat format, const ElemTextureMipData* mipData, const ElemCompressTextureMipDataOptions* options)
{
    auto stackMemoryArena = SystemGetStackMemoryArena();
    InitCompressMipDataMemoryArena();

    auto messages = SystemPushArray<ElemToolsMessage>(compressMipDataMemoryArena, 1024);
    auto messageCount = 0u;
    auto hasErrors = false;

    if (format != ElemToolsGraphicsFormat_BC7)
    {
        return { .Messages = ConstructErrorMessageSpan(compressMipDataMemoryArena, "The format of the mip data should be BC7 for now."), .HasErrors = true };
    }

    ElemCompressTextureMipDataOptions compressMipDataOptions = {};

    if (options)
    {
        compressMipDataOptions = *options;
    }

    // TODO: Fow now we don't use the GPU
    bc7enc_compress_block_init();

    bc7enc_compress_block_params bc7Params;
    bc7enc_compress_block_params_init(&bc7Params);
    bc7Params.m_uber_level = 4; // TODO: Do a global quality level option
    //bc7Params.m_uber_level = 0; // TODO: Do a global quality level option

    auto blockWidth = (mipData->Width + 3) / 4;
    auto blockHeight = (mipData->Height + 3) / 4;
    auto blockCount = blockWidth * blockHeight;

    auto compressedData = SystemPushArray<uint8_t>(compressMipDataMemoryArena, blockCount * TEXTURE_BC_BLOCK_SIZE_IN_BYTES);
    auto sourceBlockData = SystemPushArray<uint8_t>(stackMemoryArena, 4 * 4 * 4); // TODO: DEFINE

    for (uint32_t by = 0; by < blockHeight; by++)
    {
        for (uint32_t bx = 0; bx < blockWidth; bx++)
        {
            auto destinationPointer = &compressedData[(by * blockWidth + bx) * TEXTURE_BC_BLOCK_SIZE_IN_BYTES];

            for (uint32_t row = 0; row < 4; row++)
            {
                auto sy = by * 4 + row;
                sy = SystemMax(0u, SystemMin(sy, mipData->Height - 1));

                for (uint32_t col = 0; col < 4; col++)
                {
                    uint32_t sx = bx * 4 + col;
                    sx = SystemMax(0u, SystemMin(sx, mipData->Width - 1));

                    int srcIndex = (sy * mipData->Width + sx) * 4;
                    int dstIndex = (row * 4 + col) * 4; 
                    sourceBlockData[dstIndex + 0] = mipData->Data.Items[srcIndex + 0];
                    sourceBlockData[dstIndex + 1] = mipData->Data.Items[srcIndex + 1];
                    sourceBlockData[dstIndex + 2] = mipData->Data.Items[srcIndex + 2];
                    sourceBlockData[dstIndex + 3] = mipData->Data.Items[srcIndex + 3];
                }
            }

            bc7enc_compress_block(destinationPointer, sourceBlockData.Pointer, &bc7Params);
        }
    }
    
    return
    {
        .MipData = 
        { 
            .Width = mipData->Width,
            .Height = mipData->Height,
            .Data = { .Items = compressedData.Pointer, .Length = (uint32_t)compressedData.Length } 
        },
        .Messages = { .Items = messages.Pointer, .Length = messageCount },
        .HasErrors = hasErrors
    };
}
