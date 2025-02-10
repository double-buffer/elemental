#pragma once

#include "Elemental.h"
#include "SampleMath.h"
#include "SampleGpuMemory.h"
#include "SampleShader.h"
#include "Data/ShaderData.h"

typedef struct
{
    SampleShader DrawTextShader;
    SampleGpuMemory GpuMemory;
    SampleGpuBuffer TextBuffer;
    uint8_t* TextBufferData;
    uint32_t MaxTextBufferCount;
    uint32_t TextBufferCount;
    SampleGpuBuffer Draw2DCommandsBuffer;
    Draw2DCommand* Draw2DCommands;
    uint32_t Draw2DCommandCount;
    uint32_t MaxDraw2DCommandCount;
} ElemArtData;

void ElemArtInit(ElemGraphicsDevice graphicsDevice, ElemArtData* elemArtData);
void ElemArtPushText(ElemArtData* elemArtData, uint32_t x, uint32_t y, const char* text, uint32_t length, float red, float green, float blue, float alpha);
void ElemArtPushTextOld(ElemArtData* elemArtData, uint32_t x, uint32_t y, const char* format, ...);
void ElemArtRender(ElemCommandList commandList, ElemVector2 renderTargetSize, ElemArtData* elemArtData);

