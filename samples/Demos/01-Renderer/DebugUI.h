#pragma once

#include "Elemental.h"
#include "ElementalArt.h"
#include "clay.h"

typedef enum
{
    DebugUIStatisticType_Milliseconds,
    DebugUIStatisticType_Integer,
} DebugUIStatisticType;

typedef enum
{
    DebugUIStatisticGroup_General,
    DebugUIStatisticGroup_GpuPipeline,
} DebugUIStatisticGroup;

typedef struct
{
    const char* Label;
    float Value;
    float ExpectedValue;
    float ExpectedDifferenceGoodRange;
    DebugUIStatisticType Type;
    DebugUIStatisticGroup Group;
    uint32_t Level;
} DebugUIStatisticItem;

typedef struct
{
    DebugUIStatisticItem* Items;
    uint32_t Count;
} DebugUIStatistics;

typedef struct
{
    ElemGraphicsDevice GraphicsDevice;
    ElemArtData ElemArtData;
    ElemGraphicsHeap UIRenderTargetHeap;
    ElemGraphicsResource UIRenderTargetTexture;
    ElemGraphicsResourceDescriptor UIRenderTargetTextureReadDescriptor;
    uint32_t Width;
    uint32_t Height;
    float Scale;
    DebugUIStatistics Statistics;
    Clay_Arena ClayArena;
    char* TextCache;
    uint32_t TextCacheIndex;
} DebugUIData;

void InitDebugUI(ElemGraphicsDevice graphicsDevice, uint32_t width, uint32_t height, float scale, DebugUIData* debugUIData);
void ResizeDebugUI(DebugUIData* debugUIData, uint32_t width, uint32_t height);
void RenderDebugUI(ElemCommandList commandList, DebugUIData* debugUIData);

void PushDebugUIStatisticItem(DebugUIData* debugUIData, DebugUIStatisticItem item);
