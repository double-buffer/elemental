#include "ElementalTools.h"
#include "SampleUtils.h"
#include "SampleTexture.h"

int main(int argc, const char* argv[]) 
{
    // TODO: Refactor options parsing and put it in a header common
    // to tools sample and normal samples
    if (argc < 3)
    {
        printf("USAGE: TextureCompiler [options] inputfile outputfile\n");
        printf("\n");
        printf("OPTIONS:\n");
        printf("\n");
        return 0;
    }

    int32_t inputPathIndex = argc - 2;
    const char* inputPath = argv[inputPathIndex];

    int32_t outputPathIndex = argc - 1;
    const char* outputPath = argv[outputPathIndex];

    // TODO: Add more checks
    for (uint32_t i = 1; i < (uint32_t)(argc - 2); i++)
    {
        printf("Options: %s\n", argv[i]);
    }

    printf("Compiling texture: %s\n", inputPath);

    SampleInitTimer();
    double initialTimer = SampleGetTimerValueInMS();

    ElemLoadTextureResult loadTextureResult = ElemLoadTexture(inputPath, NULL);

    DisplayOutputMessages("LoadTexture", loadTextureResult.Messages);

    if (loadTextureResult.HasErrors)
    {
        return 1;
    }

    printf("Loaded texture in %.2fs\n", (SampleGetTimerValueInMS() - initialTimer) / 1000.0);

    ElemTextureMipDataSpan mipData = loadTextureResult.MipData;

    if (mipData.Length == 1)
    {
        double beforeMipGenerationTimer = SampleGetTimerValueInMS();
        printf("Generating MipData...\n");

        ElemGenerateTextureMipDataResult generateMipDataResult = ElemGenerateTextureMipData(loadTextureResult.Format, &loadTextureResult.MipData.Items[0], NULL);

        DisplayOutputMessages("GenerateTextureMipData", generateMipDataResult.Messages);

        if (generateMipDataResult.HasErrors)
        {
            return 1;
        }

        printf("GeneratedMips Count: %d\n", generateMipDataResult.MipData.Length);
        
        mipData = generateMipDataResult.MipData;
        printf("Generated MipData in %.2fs\n", (SampleGetTimerValueInMS() - beforeMipGenerationTimer) / 1000.0);
    }
    
    printf("Writing texture data to: %s\n", outputPath);
    FILE* file = fopen(outputPath, "wb");

    SampleTextureHeader textureHeader =
    {
        .FileId = { 'T', 'E', 'X', 'T', 'U', 'R', 'E' },
        .Format = SampleTextureFormat_BC7, // TODO: Don't hardcode this
        .Width = loadTextureResult.Width,
        .Height = loadTextureResult.Height,
        .MipCount = mipData.Length
    };

    fwrite(&textureHeader, sizeof(SampleTextureHeader), 1, file);

    // TODO: Get rid of malloc?
    SampleTextureDataBlockEntry* mipDataOffsets = (SampleTextureDataBlockEntry*)malloc(sizeof(SampleTextureDataBlockEntry) * mipData.Length);
    fwrite(mipDataOffsets, sizeof(SampleTextureDataBlockEntry), mipData.Length, file);

    double beforeEncodeTimer = SampleGetTimerValueInMS();
    // TODO: Add format in the logging message
    printf("Compressing data...\n");

    for (uint32_t i = 0; i < mipData.Length; i++)
    {
        ElemTextureMipData* mipLevelData = &mipData.Items[i];
        printf("Size: %dx%d\n", mipLevelData->Width, mipLevelData->Height);

        ElemCompressTextureMipDataResult compressedTextureData = ElemCompressTextureMipData(ElemToolsGraphicsFormat_BC7, mipLevelData, NULL);

        DisplayOutputMessages("CompressTextureMipData", compressedTextureData.Messages);

        if (compressedTextureData.HasErrors)
        {
            return 1;
        }
        
        uint32_t dataOffset = ftell(file);
        fwrite(compressedTextureData.MipData.Data.Items, sizeof(uint8_t), compressedTextureData.MipData.Data.Length, file);

        mipDataOffsets[i] = (SampleTextureDataBlockEntry) { .Offset = dataOffset, .SizeInBytes = ftell(file) - dataOffset };
    }

    printf("Compressed data in %.2fs\n", (SampleGetTimerValueInMS() - beforeEncodeTimer) / 1000.0);

    fseek(file, sizeof(SampleTextureHeader), SEEK_SET);
    fwrite(mipDataOffsets, sizeof(SampleTextureHeader), mipData.Length, file);

    fclose(file);
    printf("Texture compiled in %.2fs\n", (SampleGetTimerValueInMS() - initialTimer) / 1000.0);
}
