#pragma once

#include <string.h>
#include <stdlib.h>
#include "Elemental.h"

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

// -----------------------------------------------------------------------------
// I/O Functions
// -----------------------------------------------------------------------------
void CopyString(char* destination, uint32_t destinationLength, const char* source, uint32_t sourceLength)
{
    #ifdef _WIN32 
    strncpy_s(destination, destinationLength, source, sourceLength);
    #else
    strncpy(destination, source, sourceLength);
    #endif
}

void SampleGetFullPath(char* destination, const char* path)
{
    memset(destination, 0, MAX_PATH);

    char* pointer = NULL;
    ElemSystemInfo systemInfo = ElemGetSystemInfo();

    CopyString(destination, MAX_PATH, systemInfo.ApplicationPath, strlen(systemInfo.ApplicationPath));
    pointer = destination + strlen(systemInfo.ApplicationPath);

    const char* folderPrefix = "Data/";

    if (systemInfo.Platform == ElemPlatform_MacOS)
    {
        folderPrefix = "../Resources/";
    }
    else if (systemInfo.Platform == ElemPlatform_iOS)
    {
        folderPrefix = "./";
    }

    CopyString(pointer, pointer - destination, folderPrefix, strlen(folderPrefix));
    pointer = pointer + strlen(folderPrefix);

    CopyString(pointer, pointer - destination, path, strlen(path));
}

ElemDataSpan SampleReadFile(const char* filename) 
{
    char absolutePath[MAX_PATH];
    SampleGetFullPath(absolutePath, filename);

    printf("Read path: %s\n", absolutePath);

    #ifdef _WIN32
    FILE* file;
    fopen_s(&file, absolutePath, "rb");
    #else
    FILE* file = fopen(absolutePath, "rb");
    #endif

    if (file == NULL) 
    {
        return (ElemDataSpan) 
        {
            .Items = NULL,
            .Length = 0
        };
    }

    if (fseek(file, 0, SEEK_END) != 0) 
    {
        fclose(file);

        return (ElemDataSpan) 
        {
            .Items = NULL,
            .Length = 0
        };
    }

    long fileSize = ftell(file);

    if (fileSize == -1) 
    {
        fclose(file);

        return (ElemDataSpan) 
        {
            .Items = NULL,
            .Length = 0
        };
    }

    rewind(file);

    uint8_t* buffer = (uint8_t*)malloc(fileSize + 1);

    if (buffer == NULL)
    {
        fclose(file);

        return (ElemDataSpan) 
        {
            .Items = NULL,
            .Length = 0
        };
    }
    
    size_t bytesRead = fread(buffer, 1, fileSize, file);

    if (bytesRead < (size_t)fileSize) 
    {
        free(buffer);
        fclose(file);

        return (ElemDataSpan) 
        {
            .Items = NULL,
            .Length = 0
        };
    }

    fclose(file);

    return (ElemDataSpan) 
    {
        .Items = buffer,
        .Length = bytesRead
    };
}

// -----------------------------------------------------------------------------
// UI Functions
// -----------------------------------------------------------------------------

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
    globalSampleFpsCounter++;

    double endTime = SampleGetTimerValueInMS();
    globalSampleFrameCpuAverage = globalSampleFrameCpuAverage * 0.95 + (endTime - globalSampleStartTime) * 0.05;

    if (endTime - globalSampleFpsTimerStart >= 1000.0)
    {
        globalSampleCurrentFpsCounter = globalSampleFpsCounter - 1; 
        globalSampleFpsCounter = 1;
        globalSampleFpsTimerStart = SampleGetTimerValueInMS();
    }

    return (SampleFrameMeasurement)
    {
        .FrameTimeInSeconds = globalSampleFrameCpuAverage / 1000.0,
        .Fps = globalSampleCurrentFpsCounter
    };
}
