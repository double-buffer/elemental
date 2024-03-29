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

    #ifdef _WIN32 
    strncpy_s(destination, MAX_PATH, programPath, pointer + 1 - programPath);
    #else
    strncpy(destination, programPath, pointer + 1 - programPath);
    #endif
    
    pointer = destination + (pointer + 1 - programPath);

    #ifdef _WIN32
    strcpy_s(pointer, MAX_PATH - (pointer - destination), path);
    #else
    strcpy(pointer, path);
    #endif
}

char* ReadFileToString(const char* executablePath, const char* filename) 
{
    char absolutePath[MAX_PATH];
    GetFullPath(absolutePath, executablePath, filename);

    #ifdef _WIN32
    FILE* file;
    fopen_s(&file, absolutePath, "rb");
    #else
    FILE* file = fopen(absolutePath, "rb");
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
