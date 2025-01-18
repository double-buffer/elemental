#include "ElementalTools.h"
#include "SampleUtils.h"

int main(int argc, const char* argv[]) 
{
    if (argc < 3)
    {
        printf("USAGE: ShaderCompiler [options] inputfile outputfile\n");
        printf("\n");
        printf("OPTIONS:\n");
        printf("   --target-api\tTarget API to use: DirectX12, Vulkan, Metal. Default: to the default system target API.\n");
        printf("   --target-platform\tTarget Platform to use: Windows, MacOS, iOS. Default: to the default system target API.\n");
        printf("   --debug\tCompile with debug information.\n");
        printf("\n");
        return 0;
    }

    int32_t inputPathIndex = argc - 2;
    const char* inputPath = argv[inputPathIndex];

    int32_t outputPathIndex = argc - 1;
    const char* outputPath = argv[outputPathIndex];

    // TODO: Get extension by default and provide an option


    #ifdef _WIN32
    ElemToolsGraphicsApi targetApi = ElemToolsGraphicsApi_DirectX12;
    ElemToolsPlatform targetPlatform = ElemToolsPlatform_Windows;
    #elif __linux__
    ElemToolsGraphicsApi targetApi = ElemToolsGraphicsApi_Vulkan;
    ElemToolsPlatform targetPlatform = ElemToolsPlatform_Linux;
    #else
    ElemToolsGraphicsApi targetApi = ElemToolsGraphicsApi_Metal;
    ElemToolsPlatform targetPlatform = ElemToolsPlatform_MacOS;
    #endif

    bool debugMode = false;

    // TODO: Add more checks
    for (uint32_t i = 1; i < (uint32_t)(argc - 2); i++)
    {
        printf("Options: %s\n", argv[i]);

        if (strcmp(argv[i], "--target-platform") == 0)
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
        }
    }

    printf("Compiling shader: %s (DebugMode=%d)\n", inputPath, debugMode);

    ElemShaderCompilationResult compilationResult = ElemCompileShaderLibrary(targetApi, targetPlatform, inputPath, &(ElemCompileShaderOptions) { .DebugMode = debugMode });
    
    DisplayOutputMessages("ShaderCompiler", compilationResult.Messages);

    if (compilationResult.HasErrors)
    {
        return 1;
    }

    // TODO: Add timing informations
    printf("Writing shader data to: %s\n", outputPath);
    return SampleWriteDataToFile(outputPath, compilationResult.Data, false);
}
