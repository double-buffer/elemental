#include "ElementalTools.h"
#include "SampleUtils.h"

void SampleReadObjMesh(ElemToolsDataSpan data)
{
    ElemToolsDataSpan line = SampleReadLine(&data);

    while (line.Length > 0)
    {
        ElemToolsDataSpan lineParts[64]; 
        uint32_t linePartCount = 0;
        SampleSplitString(lineParts, &linePartCount, line);

        if (linePartCount > 1)
        {
            if (SampleCompareString(lineParts[0], "g"))
            {
                printf("Group!\n");
            }
            else if (SampleCompareString(lineParts[0], "v"))
            {
                printf("Vertex!\n");
            }
            else if (SampleCompareString(lineParts[0], "vn"))
            {
                printf("Normal!\n");
            }
            else if (SampleCompareString(lineParts[0], "f"))
            {
                printf("Face!\n");
            }
            else
            {
                printf("Unknown!\n");
            }
        }

        line = SampleReadLine(&data);
    }
}

int main(int argc, const char* argv[]) 
{
    if (argc < 3)
    {
        printf("USAGE: MeshCompiler [options] inputfile outputfile\n");
        printf("\n");
        printf("OPTIONS:\n");
        printf("   --meshlet-triangle-count\tTBD: TBD. Default: TBD.\n");
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

        /*if (strcmp(argv[i], "--target-platform") == 0)
        {
            const char* targetPlatformString = argv[i + 1];

            if (strcmp(targetPlatformString, "iOS") == 0)
            {
                targetPlatform = ElemToolsPlatform_iOS;
                printf("iOS platform\n");
            }
        }
        else if (strcmp(argv[i], "--target-api") == 0)
        {
            const char* targetPlatformString = argv[i + 1];

            if (strcmp(targetPlatformString, "vulkan") == 0)
            {
                targetApi = ElemToolsGraphicsApi_Vulkan;
                printf("Vulkan api\n");
            }
        }
        else if (strcmp(argv[i], "--debug") == 0)
        {
            debugMode = true;
        }*/
    }

    printf("Compiling mesh: %s\n", inputPath);

    ElemToolsDataSpan inputData = SampleReadFile(inputPath); 
    SampleReadObjMesh(inputData); 
}
