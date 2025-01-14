#include "TextureLoader.h"
#include "ToolsUtils.h"
#include "ElementalTools.h"
#include "SystemMemory.h"
#include "SystemFunctions.h"

#define DDSCAPS2_CUBEMAP 0x200
#define DDSCAPS2_VOLUME 0x200000
#define DDS_DIMENSION_TEXTURE2D 3

struct DdsPixelFormat
{
	unsigned int dwSize;
	unsigned int dwFlags;
	unsigned int dwFourCC;
	unsigned int dwRGBBitCount;
	unsigned int dwRBitMask;
	unsigned int dwGBitMask;
	unsigned int dwBBitMask;
	unsigned int dwABitMask;
};

struct DdsHeader
{
	unsigned int dwSize;
	unsigned int dwFlags;
	unsigned int dwHeight;
	unsigned int dwWidth;
	unsigned int dwPitchOrLinearSize;
	unsigned int dwDepth;
	unsigned int dwMipMapCount;
	unsigned int dwReserved1[11];
	DdsPixelFormat ddspf;
	unsigned int dwCaps;
	unsigned int dwCaps2;
	unsigned int dwCaps3;
	unsigned int dwCaps4;
	unsigned int dwReserved2;
};

struct DdsHeaderDirectX10
{
	unsigned int dxgiFormat;
	unsigned int resourceDimension;
	unsigned int miscFlag;
	unsigned int arraySize;
	unsigned int miscFlags2;
};

enum DXGI_FORMAT
{
	DXGI_FORMAT_BC1_UNORM = 71,
	DXGI_FORMAT_BC1_UNORM_SRGB = 72,
	DXGI_FORMAT_BC2_UNORM = 74,
	DXGI_FORMAT_BC2_UNORM_SRGB = 75,
	DXGI_FORMAT_BC3_UNORM = 77,
	DXGI_FORMAT_BC3_UNORM_SRGB = 78,
	DXGI_FORMAT_BC4_UNORM = 80,
	DXGI_FORMAT_BC4_SNORM = 81,
	DXGI_FORMAT_BC5_UNORM = 83,
	DXGI_FORMAT_BC5_SNORM = 84,
	DXGI_FORMAT_BC6H_UF16 = 95,
	DXGI_FORMAT_BC6H_SF16 = 96,
	DXGI_FORMAT_BC7_UNORM = 98,
	DXGI_FORMAT_BC7_UNORM_SRGB = 99,
};

uint32_t FourCC(const char value[5])
{
	return ((uint32_t)value[0] << 0) | ((uint32_t)value[1] << 8) | ((uint32_t)value[2] << 16) | ((uint32_t)value[3] << 24);
}

ElemToolsGraphicsFormat GetDdsTextureFormat(const DdsHeader* ddsHeader, const DdsHeaderDirectX10* directX10Header)
{
    // TODO: Other formats
    if (ddsHeader->ddspf.dwFourCC == FourCC("DX10"))
    {
        if (directX10Header->dxgiFormat == DXGI_FORMAT_BC7_UNORM_SRGB || directX10Header->dxgiFormat == DXGI_FORMAT_BC7_UNORM)
        {
            return ElemToolsGraphicsFormat_BC7;
        }
    }

    return ElemToolsGraphicsFormat_Unknown;
}

uint32_t GetDdsFormatBlockSize(ElemToolsGraphicsFormat format)
{
    // TODO: other formats
    if (format == ElemToolsGraphicsFormat_BC7)
    {
        return 16;
    }

    return 8;
}

ElemLoadTextureResult LoadDdsTexture(const char* path, ElemLoadTextureFileFormat fileFormat, const ElemLoadTextureOptions* options)
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

    auto currentFilePointer = objFileData.Pointer;

    if (*(uint32_t*)currentFilePointer != FourCC("DDS "))
    {
        return { .Messages = ConstructErrorMessageSpan(textureLoaderMemoryArena, "Texture is not a DDS file."), .HasErrors = true };
    }

    currentFilePointer += 4;

    auto header = (DdsHeader*)currentFilePointer;
    currentFilePointer += sizeof(DdsHeader);

    DdsHeaderDirectX10* directX10Header = nullptr;

    if (header->ddspf.dwFourCC == FourCC("DX10"))
    {
        directX10Header = (DdsHeaderDirectX10*)currentFilePointer;
        currentFilePointer += sizeof(DdsHeaderDirectX10);
    }

    if (header->dwCaps2 & (DDSCAPS2_CUBEMAP | DDSCAPS2_VOLUME))
    {
        return { .Messages = ConstructErrorMessageSpan(textureLoaderMemoryArena, "Cubemaps and volume textures are not supported."), .HasErrors = true };
    }

    if (header->ddspf.dwFourCC == FourCC("DX10") && directX10Header->resourceDimension != DDS_DIMENSION_TEXTURE2D)
    {
        return { .Messages = ConstructErrorMessageSpan(textureLoaderMemoryArena, "Only Texture2D are supported."), .HasErrors = true };
    }

    auto format = GetDdsTextureFormat(header, directX10Header);

    if (format == ElemToolsGraphicsFormat_Unknown)
    {
        return { .Messages = ConstructErrorMessageSpan(textureLoaderMemoryArena, "Unknown texture format."), .HasErrors = true };
    }

    auto width = header->dwWidth;
    auto height = header->dwHeight;
    auto mipLevels = header->dwMipMapCount;
    auto blockSize = GetDdsFormatBlockSize(format);

    auto mipData = SystemPushArray<ElemTextureMipData>(textureLoaderMemoryArena, mipLevels);

    for (uint32_t i = 0; i < mipLevels; i++)
    {
        auto mipLevelData = &mipData[i];
        printf("Processing mip level %d\n", i);

        auto mipWidth = SystemMax(1u, width >> i);
        auto mipHeight = SystemMax(1u, height >> i);

        auto dataSizeInBytes = ((mipWidth + 3) / 4) * ((mipHeight + 3) / 4) * blockSize;
        printf("Data size: %d\n", dataSizeInBytes);

        mipLevelData->Width = mipWidth;
        mipLevelData->Height = mipHeight;

        auto mipLevelRawData = SystemDuplicateBuffer(textureLoaderMemoryArena, ReadOnlySpan<uint8_t>((uint8_t*)currentFilePointer, dataSizeInBytes));
        mipLevelData->Data = { .Items = mipLevelRawData.Pointer, .Length = (uint32_t)mipLevelRawData.Length };

        currentFilePointer += dataSizeInBytes;
    }

    ResetLoadFileDataMemory();

    return 
    {
        .FileFormat = fileFormat,
        .Format = format,
        .Width = (uint32_t)width,
        .Height = (uint32_t)height,
        .MipData = { .Items = mipData.Pointer, .Length = (uint32_t)mipData.Length },
        .Messages = { .Items = messages.Pointer, .Length = messageCount },
        .HasErrors = hasErrors
    };
}
