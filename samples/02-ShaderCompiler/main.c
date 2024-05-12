#include "ElementalTools.h"
#include <stdlib.h>

#ifndef _WIN32
#define MAX_PATH 255
#include <sys/time.h>
#else
#include <windows.h>
#endif

char* ReadFileToString(const char* filename) 
{
    #ifdef _WIN32
    FILE* file;
    fopen_s(&file, filename, "rb");
    #else
    FILE* file = fopen(filename, "rb");
    #endif

    if (file == NULL) 
    {
        return NULL;
    }

    if (fseek(file, 0, SEEK_END) != 0) 
    {
        fclose(file);
        return NULL;
    }

    long fileSize = ftell(file);

    if (fileSize == -1) 
    {
        fclose(file);
        return NULL;
    }

    rewind(file);

    char* buffer = (char*)malloc(fileSize + 1);

    if (buffer == NULL)
    {
        fclose(file);
        return NULL;
    }
    
    size_t bytesRead = fread(buffer, 1, fileSize, file);

    if (bytesRead < (size_t)fileSize) 
    {
        free(buffer);
        fclose(file);
        return NULL;
    }

    buffer[fileSize] = '\0';
    fclose(file);

    return buffer;
}

int WriteDataToFile(const char* filename, ElemToolsDataSpan data) 
{
    printf("Length:%s %d\n", filename, data.Length);
    if (filename == NULL || data.Length == 0) 
    {
        printf("ERROR 1\n");
        return -1;
    }

    #ifdef _WIN32
    FILE* file;
    fopen_s(&file, filename, "wb");
    #else
    FILE* file = fopen(filename, "wb");
    #endif

    if (file == NULL) 
    {
        printf("ERROR 2\n");
        return -1;
    }

    size_t bytesWritten = fwrite(data.Items, 1, data.Length, file);
    fclose(file);

    if (bytesWritten < data.Length) 
    {
        printf("ERROR 3\n");
        return -1; // Return -1 if not all bytes were written
    }

    return 0; // Success
}

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

    char* shaderSource = ReadFileToString(inputPath);
    ElemShaderSourceData shaderSourceData = { .ShaderLanguage = ElemShaderLanguage_Hlsl, .Data = { .Items = (uint8_t*)shaderSource, .Length = strlen(shaderSource) } };
    ElemShaderCompilationResult compilationResult = ElemCompileShaderLibrary(targetApi, targetPlatform, &shaderSourceData, &(ElemCompileShaderOptions) { .DebugMode = debugMode });

    for (uint32_t i = 0; i < compilationResult.Messages.Length; i++)
    {
        printf("Compil msg (%d): %s\n", compilationResult.Messages.Items[i].Type, compilationResult.Messages.Items[i].Message);
    }

    if (compilationResult.HasErrors)
    {
        printf("Error while compiling shader!\n");
        return 1;
    }

    printf("Writing shader data to: %s\n", outputPath);
    return WriteDataToFile(outputPath, compilationResult.Data);
}
