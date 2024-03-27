#pragma once

#include <string.h>
#include <stdlib.h>
#include "Elemental.h"

#ifndef _WIN32
#define MAX_PATH 255
#endif 

void GetFullPath(char* destination, const char* programPath, const char* path)
{
    char* pointer = strrchr(programPath, '\\');

    if (!pointer)
    {
        pointer = strrchr(programPath, '/');
    }

    strncpy(destination, programPath, pointer + 1 - programPath);
    
    pointer = destination + (pointer + 1 - programPath);
    strcpy(pointer, path);
}

// TODO: Extract not relevant files to a common sample header to we can focus on the purpose of the sample
char* ReadFileToString(const char* executablePath, const char* filename) 
{
    char absolutePath[MAX_PATH];
    GetFullPath(absolutePath, executablePath, filename);

    FILE* file = fopen(absolutePath, "rb");

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

const char* GetGraphicsApiLabel(ElemGraphicsApi graphicsApi)
{
    switch (graphicsApi)
    {
        case ElemGraphicsApi_DirectX12:
            return "DirectX12";

        case ElemGraphicsApi_Vulkan:
            return "Vulkan";

        case ElemGraphicsApi_Metal:
            return "Metal";
    }

    return "Unknown";
}
