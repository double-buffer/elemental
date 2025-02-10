#pragma once

#include "Elemental.h"
#include "ElementalArt.h"
#include "clay.h"

typedef struct
{
    uint32_t Fps;
    float CpuFrameTimeMS;
    float GpuFrameTimeMS;
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
} DebugUIData;

void InitDebugUI(ElemGraphicsDevice graphicsDevice, uint32_t width, uint32_t height, float scale, DebugUIData* debugUIData);
void ResizeDebugUI(DebugUIData* debugUIData, uint32_t width, uint32_t height);
void RenderDebugUI(ElemCommandList commandList, DebugUIData* debugUIData);
