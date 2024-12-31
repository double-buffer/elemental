#pragma once

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef ElemToolsAPI
typedef ElemToolsDataSpan ElemDataSpan;
#else
#include "../Elemental/Elemental.h"
#endif 

#ifndef _WIN32
#define MAX_PATH 255
#include <sys/time.h>
#include <time.h>
#else
#include <windows.h>
#endif

#ifdef __APPLE__
    #include "TargetConditionals.h"
#endif

uint64_t SampleMegaBytesToBytes(uint64_t value)
{
    return value * 1024 * 1024;
}

uint64_t SampleGigaBytesToBytes(uint64_t value)
{
    return SampleMegaBytesToBytes(value) * 1024;
}

uint32_t SampleAlignValue(uint32_t value, uint32_t alignment)
{
    return (value + alignment - 1) & ~(alignment - 1);
}

// -----------------------------------------------------------------------------
// I/O Functions
// -----------------------------------------------------------------------------
void SampleGetFullPath(char* destination, const char* path, bool prefixData)
{
    // TODO: Pass destination length
    memset(destination, 0, MAX_PATH);

    char* pointer = destination;

    #ifndef ElemToolsAPI
    ElemSystemInfo systemInfo = ElemGetSystemInfo();

    strncpy(destination, systemInfo.ApplicationPath, strlen(systemInfo.ApplicationPath));
    pointer = destination + strlen(systemInfo.ApplicationPath);

    const char* folderPrefix = "./";

    if (prefixData)
    {
        if (systemInfo.Platform == ElemPlatform_MacOS)
        {
            folderPrefix = "../Resources/";
        }
        else if (systemInfo.Platform == ElemPlatform_iOS)
        {
            folderPrefix = "./";
        }
        else
    {
            folderPrefix = "Data/";
        }
    }

    strncpy(pointer, folderPrefix, strlen(folderPrefix));
    pointer = pointer + strlen(folderPrefix);
    #endif

    strncpy(pointer, path, strlen(path));
}

FILE* SampleOpenFile(const char* filename, bool prefixData)
{
    char absolutePath[MAX_PATH];
    SampleGetFullPath(absolutePath, filename, prefixData);

    //printf("Read path: %s\n", absolutePath);

    return fopen(absolutePath, "rb");
}

// TODO: To remove?
ElemDataSpan SampleReadFile(const char* filename, bool prefixData) 
{
    FILE* file = SampleOpenFile(filename, prefixData);
 
    if (file == NULL) 
    {
        return (ElemDataSpan) {};
    }

    if (fseek(file, 0, SEEK_END) != 0) 
    {
        fclose(file);
        return (ElemDataSpan) {};
    }

    long fileSize = ftell(file);

    if (fileSize == -1) 
    {
        fclose(file);
        return (ElemDataSpan) {};
    }

    rewind(file);

    uint8_t* buffer = (uint8_t*)malloc(fileSize + 1);
    memset(buffer, 0, fileSize + 1);

    if (buffer == NULL)
    {
        fclose(file);
        return (ElemDataSpan) {};
    }
    
    size_t bytesRead = fread(buffer, 1, fileSize, file);

    if (bytesRead < (size_t)fileSize) 
    {
        free(buffer);
        fclose(file);
        return (ElemDataSpan) {};
    }

    fclose(file);

    return (ElemDataSpan) 
    {
        .Items = buffer,
        .Length = bytesRead
    };
}

// TODO: Remove those?
int SampleWriteDataToFile(const char* filename, ElemDataSpan data, bool append) 
{
    const char* fileMode = append ? "ab" : "wb"; 

    FILE* file = fopen(filename, fileMode);

    if (file == NULL) 
    {
        printf("ERROR 2\n");
        return -1;
    }

    size_t bytesWritten = fwrite(data.Items, 1, data.Length, file);
    fclose(file);

    if (bytesWritten < data.Length) 
    {
        printf("ERROR 3: %zu\n", bytesWritten);
        return -1; // Return -1 if not all bytes were written
    }

    return 0; // Success
}

int SampleWriteDataToApplicationFile(const char* filename, ElemDataSpan data, bool append) 
{
    char absolutePath[MAX_PATH];
    SampleGetFullPath(absolutePath, filename, false);

    return SampleWriteDataToFile(absolutePath, data, append);
}

// TODO: Add Sample prefix
void ReplaceFileExtension(const char* path, const char* extension, char* destination, uint32_t destinationSize)
{
    assert(destinationSize >= strlen(path));
    memset(destination, 0, destinationSize);

    const char* extensionSeparator = strrchr(path, '.');
    uint32_t prefixLength = extensionSeparator ? (extensionSeparator - path) : strlen(path);

    for (uint32_t i = 0; i < prefixLength; i++)
    {
        destination[i] = path[i] == '\\' ? '/' : path[i];
    }
    
    strncat(destination, extension, prefixLength);
}

void GetFileDirectory(const char* path, char* destination, uint32_t destinationSize)
{
    assert(destinationSize >= strlen(path));
    memset(destination, 0, destinationSize);
    
    for (uint32_t i = 0; i < strlen(path); i++)
    {
        destination[i] = path[i] == '\\' ? '/' : path[i];
    }

    char* lastSeparator = strrchr(destination, '/');

    if (lastSeparator) 
    {
        *(lastSeparator + 1) = '\0';
    }
    else 
    {
        destination[0] = '\0';
    }
}

void GetRelativeResourcePath(const char* mainPath, const char* path, const char* extension, char* destination, uint32_t destinationSize)
{
    assert(destinationSize >= strlen(path));
    memset(destination, 0, destinationSize);

    char mainDirectory[destinationSize];
    GetFileDirectory(mainPath, mainDirectory, destinationSize);

    char resourcePath[MAX_PATH];
    ReplaceFileExtension(path, extension, resourcePath, sizeof(resourcePath));

    size_t mainDirectoryLength = strlen(mainDirectory);
    const char* startPath = resourcePath;

    if (strncmp(resourcePath, mainDirectory, mainDirectoryLength) == 0) 
    {
        startPath = resourcePath + mainDirectoryLength;
    }

    strncpy(destination, startPath, startPath - resourcePath);
}

// -----------------------------------------------------------------------------
// UI Functions
// -----------------------------------------------------------------------------

#ifdef ElemAPI
const char* SampleGetPlatformLabel(ElemPlatform platform)
{
    switch (platform)
    {
        case ElemPlatform_Windows:
            return "Windows";

        case ElemPlatform_MacOS:
            return "MacOS";

        case ElemPlatform_iOS:
            return "iOS";

        case ElemPlatform_Linux:
            return "Linux";
    }

    return "Unknown";
}

const char* SampleGetGraphicsApiLabel(ElemGraphicsApi graphicsApi)
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

void FormatMemorySize(uint64_t bytes, char* outputBuffer, size_t bufferSize) 
{
    const char* suffixes[] = { "B", "KB", "MB", "GB", "TB" }; // Extend if more are needed
    double size = bytes;
    size_t i = 0;

    while (size >= 1024 && i < sizeof(suffixes)/sizeof(suffixes[0]) - 1) 
    {
        size /= 1024.0;
        i++;
    }

    snprintf(outputBuffer, bufferSize, "%.2f %s", size, suffixes[i]);
}

void SampleSetWindowTitle(ElemWindow window, const char* applicationName, ElemGraphicsDevice graphicsDevice, double frameTimeInSeconds, uint32_t fps)
{
    ElemWindowSize renderSize = ElemGetWindowRenderSize(window);
    
    ElemSystemInfo systemInfo = ElemGetSystemInfo();
    ElemGraphicsDeviceInfo graphicsDeviceInfo = ElemGetGraphicsDeviceInfo(graphicsDevice);

    char memoryFormatted[64];
    FormatMemorySize(graphicsDeviceInfo.AvailableMemory, memoryFormatted, sizeof(memoryFormatted));

    char titleFormatted[256];
    snprintf(titleFormatted, sizeof(titleFormatted), "%s FPS: %u / Cpu FrameTime: %.2f (Elemental=%s, RenderSize=%ux%u@%.1f, GraphicsDevice=%s, GraphicsApi=%s, Platform=%s, AvailableMemory=%s)", 
                        applicationName,
                        fps,
                        frameTimeInSeconds * 1000.0,
                        ELEM_VERSION_LABEL,
                        renderSize.Width,
                        renderSize.Height,
                        renderSize.UIScale,
                        graphicsDeviceInfo.DeviceName, 
                        SampleGetGraphicsApiLabel(graphicsDeviceInfo.GraphicsApi),
                        SampleGetPlatformLabel(systemInfo.Platform),
                        memoryFormatted);
    ElemSetWindowTitle(window, titleFormatted);
}
#endif

// -----------------------------------------------------------------------------
// Timing Functions
// -----------------------------------------------------------------------------

uint64_t globalSampleTimerFrequency;
uint64_t globalSampleTimerBaseCounter;

void SampleInitTimer(void)
{
    #ifdef _WIN32
    QueryPerformanceFrequency((LARGE_INTEGER*) &globalSampleTimerFrequency);
    QueryPerformanceCounter((LARGE_INTEGER*) &globalSampleTimerBaseCounter);
    #else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    globalSampleTimerBaseCounter = (uint64_t)ts.tv_sec * 1000000000 + (uint64_t)ts.tv_nsec;
    globalSampleTimerFrequency = 1000000;
    #endif
}

double SampleGetTimerValueInMS(void)
{
    #ifdef _WIN32
    uint64_t value;
    QueryPerformanceCounter((LARGE_INTEGER*) &value);

    return ((double)(value - globalSampleTimerBaseCounter) / globalSampleTimerFrequency) * 1000.0;
    #else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    uint64_t value = (uint64_t)ts.tv_sec * 1000000000 + (uint64_t)ts.tv_nsec;
    return  (double)(value - globalSampleTimerBaseCounter) / globalSampleTimerFrequency;
    #endif
}

typedef struct
{
    double FrameTimeInSeconds;
    uint32_t Fps;
    bool HasNewData;
} SampleFrameMeasurement;

double globalSampleFrameCpuAverage = 0.0;
double globalSampleFpsTimerStart = 0.0;
uint32_t globalSampleCurrentFpsCounter = 0;
uint32_t globalSampleFpsCounter = 0;
double globalSampleStartTime = 0.0;

void SampleStartFrameMeasurement(void)
{
    if (globalSampleTimerFrequency == 0.0)
    {
        SampleInitTimer();
    }

    globalSampleStartTime = SampleGetTimerValueInMS();
}

SampleFrameMeasurement SampleEndFrameMeasurement(void)
{
    bool newData = globalSampleFpsCounter == 0;
    globalSampleFpsCounter++;

    double endTime = SampleGetTimerValueInMS();
    globalSampleFrameCpuAverage = globalSampleFrameCpuAverage * 0.95 + (endTime - globalSampleStartTime) * 0.05;

    if (endTime - globalSampleFpsTimerStart >= 1000.0)
    {
        globalSampleCurrentFpsCounter = globalSampleFpsCounter - 1; 
        globalSampleFpsCounter = 1;
        globalSampleFpsTimerStart = SampleGetTimerValueInMS();
        newData = true;
    }

    return (SampleFrameMeasurement)
    {
        .FrameTimeInSeconds = globalSampleFrameCpuAverage / 1000.0,
        .Fps = globalSampleCurrentFpsCounter,
        .HasNewData = newData
    };
}


// -----------------------------------------------------------------------------
// Settings Functions
// -----------------------------------------------------------------------------

typedef struct
{
    bool PreferVulkan;
    bool PreferFullScreen;
    bool DisableDiagnostics;
} SampleAppSettings;

SampleAppSettings SampleParseAppSettings(int argc, const char* argv[])
{
    SampleAppSettings appSettings = {};

    for (int32_t i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--vulkan") == 0)
        {
            appSettings.PreferVulkan = true;
        }

        if (strcmp(argv[i], "--fullscreen") == 0)
        {
            appSettings.PreferFullScreen = true;
        }

        if (strcmp(argv[i], "--disable-diagnostics") == 0)
        {
            appSettings.DisableDiagnostics = true;
        }
    }
    
    return appSettings;
}

#ifdef ElemToolsAPI
void DisplayOutputMessages(const char* prefix, ElemToolsMessageSpan messages)
{
    for (uint32_t i = 0; i < messages.Length; i++)
    {
        ElemToolsMessage* message = &messages.Items[i];

        printf("[");
        printf("\033[36m%s\033[0m]", prefix);

        switch (message->Type)
        {
            case ElemToolsMessageType_Error:
                printf("\033[31m Error:");
                break;

            case ElemToolsMessageType_Warning:
                printf("\033[33m Warning:");
                break;

            default:
                printf("\033[0m");
        }

        printf(" %s\033[0m\n", message->Message);
        fflush(stdout);
    }
}
#endif
