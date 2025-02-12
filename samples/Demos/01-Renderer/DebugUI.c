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

    debugUIData->Statistics.Items = (DebugUIStatisticItem*)malloc(1000 * sizeof(DebugUIStatisticItem));
    debugUIData->Statistics.Count = 0;

    debugUIData->TextCache = (char*)malloc(SampleMegaBytesToBytes(16));
    debugUIData->TextCacheIndex = 0;

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

Clay_Color COLOR_DEFAULT = { 255, 255, 255, 255};
Clay_Color COLOR_GOOD = { 0, 255, 0, 255};
Clay_Color COLOR_WARNING = { 255, 255, 0, 255};
Clay_Color COLOR_BAD = { 255, 0, 0, 255};

void RenderStatisticItem(DebugUIData* debugUIData, const DebugUIStatisticItem* item, float parentValue)
{
    Clay_String clayString = { .length = strlen(item->Label), .chars = item->Label };
    CLAY_TEXT(clayString, CLAY_TEXT_CONFIG({ .textColor = COLOR_DEFAULT }));

    CLAY_TEXT(CLAY_STRING(": "), CLAY_TEXT_CONFIG({ .textColor = COLOR_DEFAULT }));
 
    Clay_Color valueColor = COLOR_DEFAULT;

    if (item->ExpectedValue > 0)
    {
        float valueDifferencePercent = item->Value / item->ExpectedValue;
        valueColor = COLOR_GOOD;

        float expectedDifferenceGoodRange = item->ExpectedDifferenceGoodRange;

        if (expectedDifferenceGoodRange == 0.0f)
        {
            expectedDifferenceGoodRange = 0.8f;
        }

        if (expectedDifferenceGoodRange < 1.0f)
        {
            if (valueDifferencePercent < 0.5f)
            {
                valueColor = COLOR_BAD;
            }
            else if (valueDifferencePercent < expectedDifferenceGoodRange)
            {
                valueColor = COLOR_WARNING;
            }
        }
        else
        {
            if (valueDifferencePercent > 1.5f)
            {
                valueColor = COLOR_BAD;
            }
            else if (valueDifferencePercent > expectedDifferenceGoodRange)
            {
                valueColor = COLOR_WARNING;
            }
        }
    }

    char* tmp = &debugUIData->TextCache[debugUIData->TextCacheIndex];

    if (item->Type == DebugUIStatisticType_Integer)
    {
        snprintf(tmp, 255, "%d", (uint32_t)item->Value);
    }
    else if (item->Type == DebugUIStatisticType_Milliseconds)
    {
        snprintf(tmp, 255, "%.2f ms", item->Value);
    }

    clayString = (Clay_String){ .length = strlen(tmp), .chars = tmp };
    CLAY_TEXT(clayString, CLAY_TEXT_CONFIG({ .textColor = valueColor, .hashStringContents = true }));
    debugUIData->TextCacheIndex += strlen(tmp);

    if (parentValue > 0.0f && item->Level > 0)
    {
        float percentage = item->Value / parentValue * 100.0f;

        char* tmp = &debugUIData->TextCache[debugUIData->TextCacheIndex];
        snprintf(tmp, 255, " (%.2f %%)", percentage);

        clayString = (Clay_String){ .length = strlen(tmp), .chars = tmp };
        CLAY_TEXT(clayString, CLAY_TEXT_CONFIG({ .textColor = valueColor, .hashStringContents = true }));
        debugUIData->TextCacheIndex += strlen(tmp);
    }
}

void RenderDebugUI(ElemCommandList commandList, DebugUIData* debugUIData)
{
    Clay_BeginLayout();

    CLAY({ .layout = { .padding = CLAY_PADDING_ALL(10), .layoutDirection = CLAY_TOP_TO_BOTTOM } }) 
    {
        CLAY() 
        {
            for (uint32_t i = 0; i < debugUIData->Statistics.Count; i++)
            {
                DebugUIStatisticItem* item = &debugUIData->Statistics.Items[i];

                if (item->Group != DebugUIStatisticGroup_General)
                {
                    continue;
                }

                if (i > 0)
                {
                    CLAY_TEXT(CLAY_STRING(" - "), CLAY_TEXT_CONFIG({ .textColor = COLOR_DEFAULT }));
                }

                RenderStatisticItem(debugUIData, item, 0.0f);
            }
        }

        CLAY({ .layout = { .padding = { 0, 0, 16, 0 }, .layoutDirection = CLAY_TOP_TO_BOTTOM } }) 
        {
            float parentValues[16];

            for (uint32_t i = 0; i < debugUIData->Statistics.Count; i++)
            {
                DebugUIStatisticItem* item = &debugUIData->Statistics.Items[i];

                if (item->Group != DebugUIStatisticGroup_GpuPipeline)
                {
                    continue;
                }

                parentValues[item->Level] = item->Value;

                CLAY({ .layout = { .padding = { item->Level * 16, 0, 0, 0 } } }) 
                {
                    RenderStatisticItem(debugUIData, item, item->Level > 0 ? parentValues[item->Level - 1] : 0.0f);
                }
            }
        }
    }

    Clay_RenderCommandArray renderCommands = Clay_EndLayout();

    ClayProcessRenderCommands(&renderCommands, &debugUIData->ElemArtData);

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

    debugUIData->TextCacheIndex = 0;
    debugUIData->Statistics.Count = 0;
}

void PushDebugUIStatisticItem(DebugUIData* debugUIData, DebugUIStatisticItem item)
{
    debugUIData->Statistics.Items[debugUIData->Statistics.Count++] = item;
}
