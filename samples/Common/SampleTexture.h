#pragma once

#include <stdint.h>

typedef enum
{
    SampleTextureFormat_Unknown = 0,
    SampleTextureFormat_BC7 = 1
} SampleTextureFormat;

typedef struct
{
    char FileId[7];
    SampleTextureFormat Format;
    uint32_t Width;
    uint32_t Height;
    uint32_t MipCount;
} SampleTextureHeader;

typedef struct
{
    uint32_t Offset;
    uint32_t SizeInBytes;
} SampleTextureDataBlockEntry;
