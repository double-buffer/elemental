#include "DebugUI.h"

#include "SampleGpuMemory.h"
#include "ElementalArt.h"

#define CLAY_IMPLEMENTATION
#include "clay.h"

void HandleClayErrors(Clay_ErrorData errorData) {
    // See the Clay_ErrorData struct for more information
    printf("CLAY: %s\n", errorData.errorText.chars);
    switch(errorData.errorType) {
        // etc
    }
}

Clay_Dimensions ClayMeasureText(Clay_StringSlice text, Clay_TextElementConfig* config, void* userData) 
{
    return (Clay_Dimensions) 
    {
        .width = text.length * 8,
        .height = 16
    };
}

void ClayProcessRenderCommands(const Clay_RenderCommandArray* renderCommands, ElemArtData* elemArtData)
{
    for (int i = 0; i < renderCommands->length; i++) 
    {
        Clay_RenderCommand* renderCommand = &renderCommands->internalArray[i];

        switch (renderCommand->commandType) 
        {
            // TODO: Bounding box
            case CLAY_RENDER_COMMAND_TYPE_TEXT:
            {
                Clay_StringSlice stringContents = renderCommand->renderData.text.stringContents;
                ElemArtPushText(elemArtData, renderCommand->boundingBox.x, renderCommand->boundingBox.y, stringContents.chars, stringContents.length, 
                                    renderCommand->renderData.text.textColor.r,
                                    renderCommand->renderData.text.textColor.g,
                                    renderCommand->renderData.text.textColor.b,
                                    renderCommand->renderData.text.textColor.a);
                break;
            }
        }
    }
}

void InitDebugUI(ElemGraphicsDevice graphicsDevice, uint32_t width, uint32_t height, float scale, DebugUIData* debugUIData)
{
    debugUIData->GraphicsDevice =  graphicsDevice;
    ElemArtInit(graphicsDevice, &debugUIData->ElemArtData);

    debugUIData->UIRenderTargetHeap = ElemCreateGraphicsHeap(graphicsDevice, SampleMegaBytesToBytes(128), &(ElemGraphicsHeapOptions) { .HeapType = ElemGraphicsHeapType_Gpu });
    debugUIData->Scale = scale;

    uint64_t totalMemorySize = Clay_MinMemorySize();
    debugUIData->ClayArena = Clay_CreateArenaWithCapacityAndMemory(totalMemorySize, malloc(totalMemorySize));

    Clay_Initialize(debugUIData->ClayArena, (Clay_Dimensions) { width, height }, (Clay_ErrorHandler) { HandleClayErrors });
    Clay_SetMeasureTextFunction(ClayMeasureText, NULL);
    ResizeDebugUI(debugUIData, width, height);
}

void ResizeDebugUI(DebugUIData* debugUIData, uint32_t width, uint32_t height)
{
    if (debugUIData->UIRenderTargetTexture != ELEM_HANDLE_NULL)
    {
        ElemFreeGraphicsResourceDescriptor(debugUIData->UIRenderTargetTextureReadDescriptor, NULL);
        ElemFreeGraphicsResource(debugUIData->UIRenderTargetTexture, NULL);
    }

    printf("Creating UI render texture...\n");

    ElemGraphicsResourceInfo resourceInfo = ElemCreateTexture2DResourceInfo(debugUIData->GraphicsDevice, width, height, 1, ElemGraphicsFormat_R32G32B32A32_FLOAT, ElemGraphicsResourceUsage_RenderTarget,
                                                                            &(ElemGraphicsResourceInfoOptions) { 
                                                                                .DebugName = "UIRenderTarget" 
                                                                            });

    debugUIData->UIRenderTargetTexture = ElemCreateGraphicsResource(debugUIData->UIRenderTargetHeap, 0, &resourceInfo);
    debugUIData->UIRenderTargetTextureReadDescriptor = ElemCreateGraphicsResourceDescriptor(debugUIData->UIRenderTargetTexture, ElemGraphicsResourceDescriptorUsage_Read, NULL);

    debugUIData->Width = width;
    debugUIData->Height = height;

    Clay_SetLayoutDimensions((Clay_Dimensions) { width, height });
}

void RenderDebugUI(ElemCommandList commandList, DebugUIData* debugUIData)
{
    Clay_BeginLayout();

    Clay_TextElementConfig* defaultTextStyle = CLAY_TEXT_CONFIG({ .textColor = { 255, 255, 255, 255 } });
    Clay_TextElementConfig* goodTextStyle = CLAY_TEXT_CONFIG({ .textColor = { 0, 255, 0, 255} });

    CLAY({ .layout = { .padding = CLAY_PADDING_ALL(100) } }) 
    {
        CLAY_TEXT(CLAY_STRING("FPS: "), defaultTextStyle);

        char tmp[255];
        itoa(debugUIData->Statistics.Fps, tmp, 10);

        Clay_String cs = { .length = strlen(tmp), .chars = tmp };
        CLAY_TEXT(cs, goodTextStyle);
    }

    // All clay layouts are declared between Clay_BeginLayout and Clay_EndLayout
    Clay_RenderCommandArray renderCommands = Clay_EndLayout();

    ClayProcessRenderCommands(&renderCommands, &debugUIData->ElemArtData);

    ElemArtPushTextOld(&debugUIData->ElemArtData, 10, 10, "FPS: %u - Cpu: %.2f ms - Gpu: %.2f ms", debugUIData->Statistics.Fps, debugUIData->Statistics.CpuFrameTimeMS);

    ElemBeginRenderPass(commandList, &(ElemBeginRenderPassParameters) {
        .RenderTargets = 
        {
            .Items = (ElemRenderPassRenderTarget[]) { 
            {
                .RenderTarget = debugUIData->UIRenderTargetTexture,
                .LoadAction = ElemRenderPassLoadAction_Clear
            }},
            .Length = 1
        }
    });
        
    ElemArtRender(commandList, (ElemVector2){ debugUIData->Width / debugUIData->Scale, debugUIData->Height / debugUIData->Scale }, &debugUIData->ElemArtData);
    ElemEndRenderPass(commandList);
}
