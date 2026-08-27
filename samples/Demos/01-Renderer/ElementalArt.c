#include "ElementalArt.h"
#include "SampleGpuMemory.h"

void ElemArtInit(ElemGraphicsDevice graphicsDevice, ElemArtData* elemArtData)
{
    elemArtData->GpuMemory = SampleCreateGpuMemory(graphicsDevice, ElemGraphicsHeapType_GpuUpload, SampleMegaBytesToBytes(128));

    elemArtData->DrawTextShader = SampleCompileGraphicsShader(graphicsDevice, "DrawText.shader", &(ElemGraphicsPipelineStateParameters) {
        .DebugName = "DrawText PSO",
        .MeshShaderFunction = "MeshMain",
        .PixelShaderFunction = "PixelMain",
        .RenderTargets = { .Items = (ElemGraphicsPipelineStateRenderTarget[]) {
        { 
            .Format = ElemGraphicsFormat_R32G32B32A32_FLOAT,
        }}, .Length = 1 },
    });

    elemArtData->MaxTextBufferCount = 1024;
    elemArtData->TextBufferData = (uint8_t*)malloc(elemArtData->MaxTextBufferCount);
    elemArtData->TextBuffer = SampleCreateGpuBuffer(&elemArtData->GpuMemory, elemArtData->MaxTextBufferCount, ElemGraphicsResourceUsage_Read, "DrawTextBuffer");
    elemArtData->TextBufferCount = 0;

    elemArtData->MaxDraw2DCommandCount = 1024;
    elemArtData->Draw2DCommands = (Draw2DCommand*)malloc(elemArtData->MaxDraw2DCommandCount);
    elemArtData->Draw2DCommandsBuffer = SampleCreateGpuBuffer(&elemArtData->GpuMemory, elemArtData->MaxDraw2DCommandCount, ElemGraphicsResourceUsage_Read, "Draw2DCommandsBuffer");
    elemArtData->Draw2DCommandCount = 0;
}

// TODO: Bounding box
// TODO: Option struct
void ElemArtPushText(ElemArtData* elemArtData, uint32_t x, uint32_t y, const char* text, uint32_t length, float red, float green, float blue, float alpha)
{
    *((uint32_t*)&elemArtData->TextBufferData[elemArtData->TextBufferCount]) = length;
    strncpy((char*)&elemArtData->TextBufferData[elemArtData->TextBufferCount + 4], text, length);

    Draw2DCommand command = 
    {
        .Type = Draw2DCommandType_Text,
        .CommandDataOffset = elemArtData->TextBufferCount,
        .PositionX = x,
        .PositionY = y,
        .ColorRed = red,
        .ColorGreen = green,
        .ColorBlue = blue,
        .ColorAlpha = alpha,
    };

    elemArtData->Draw2DCommands[elemArtData->Draw2DCommandCount++] = command;
    elemArtData->TextBufferCount += SampleAlignValue(length + 4, sizeof(uint32_t));
}

void ElemArtRender(ElemCommandList commandList, ElemVector2 renderTargetSize, ElemArtData* elemArtData)
{
    ElemUploadGraphicsBufferData(elemArtData->TextBuffer.Buffer, 0, (ElemDataSpan) { .Items = (uint8_t*)elemArtData->TextBufferData, .Length = elemArtData->TextBufferCount });

    ElemUploadGraphicsBufferData(elemArtData->Draw2DCommandsBuffer.Buffer, 0, (ElemDataSpan) { .Items = (uint8_t*)elemArtData->Draw2DCommands, .Length = elemArtData->Draw2DCommandCount * sizeof(Draw2DCommand) });

    // TODO: change that
    ElemBindPipelineState(commandList, elemArtData->DrawTextShader.PipelineState); 

    DrawTextShaderParameters parameters = 
    {
        .TextBufferIndex = elemArtData->TextBuffer.ReadDescriptor,
        .Draw2DCommandsBufferIndex = elemArtData->Draw2DCommandsBuffer.ReadDescriptor,
        .CommandCount = elemArtData->Draw2DCommandCount,
        .RenderTargetSize = renderTargetSize,
    };

    ElemPushPipelineStateConstants(commandList, 0, (ElemDataSpan) { .Items = (uint8_t*)&parameters, .Length = sizeof(RaytracingShaderParameters) });

    ElemDispatchMesh(commandList, 1, 1, 1);
    elemArtData->TextBufferCount = 0;
    elemArtData->Draw2DCommandCount = 0;
}
