#include "ElementalTools.h"
#include "SampleUtils.h"

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

        ElemGenerateTextureMipDataResult generateMipDataResult = ElemGenerateTextureMipData(loadTextureResult.Format, loadTextureResult.MipData.Items[0], NULL);

        printf("GeneratedMips Count: %d\n", generateMipDataResult.MipData.Length);

        
        mipData = generateMipDataResult.MipData;
        printf("Generated MipData in %.2fs\n", (SampleGetTimerValueInMS() - beforeMipGenerationTimer) / 1000.0);
    }

    double beforeEncodeTimer = SampleGetTimerValueInMS();
    printf("Encoding data...\n");

    for (uint32_t i = 0; i < mipData.Length; i++)
    {
        ElemTextureMipData* mipLevelData = &mipData.Items[i];
        printf("Size: %dx%d\n", mipLevelData->Width, mipLevelData->Height);
    }

    printf("Encoded data in %.2fs\n", (SampleGetTimerValueInMS() - beforeEncodeTimer) / 1000.0);

    printf("Writing texture data to: %s\n", outputPath);

    FILE* file = fopen(outputPath, "wb");
    fclose(file);
    printf("Texture compiled in %.2fs\n", (SampleGetTimerValueInMS() - initialTimer) / 1000.0);
}
