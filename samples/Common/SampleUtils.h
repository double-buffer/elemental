#pragma once

#include <string.h>
#include <stdlib.h>
#include "Elemental.h"

#ifndef _WIN32
#define MAX_PATH 255
#include <sys/time.h>
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

void SampleGetFullPath(char* destination, const char* programPath, const char* path)
{
    memset(destination, 0, MAX_PATH);

    char* pointer = strrchr(programPath, '\\');

    if (!pointer)
    {
        pointer = strrchr(programPath, '/');
    }

    uint32_t programPathLength = pointer + 1 - programPath;
    CopyString(destination, MAX_PATH, programPath, programPathLength);
    pointer = destination + programPathLength;

    #if TARGET_OS_IOS
    const char* folderPrefix = "./";
    #elif TARGET_OS_MAC
    const char* folderPrefix = "../Resources/";
    #else
    const char* folderPrefix = "Data/";
    #endif
    
    CopyString(pointer, MAX_PATH, folderPrefix, strlen(folderPrefix));
    pointer = pointer + strlen(folderPrefix);

    CopyString(pointer, MAX_PATH, path, strlen(path));
}

ElemDataSpan SampleReadFile(const char* executablePath, const char* filename) 
{
    char absolutePath[MAX_PATH];
    SampleGetFullPath(absolutePath, executablePath, filename);

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

void SampleSetWindowTitle(ElemWindow window, const char* applicationName, ElemGraphicsDeviceInfo graphicsDeviceInfo, double frameTimeInSeconds, uint32_t fps)
{
    ElemWindowSize renderSize = ElemGetWindowRenderSize(window);

    char temp[256];
    sprintf(temp, "%s FPS: %u / Cpu FrameTime: %.2f (RenderSize: %ux%u@%.1f, GraphicsDevice: DeviceName=%s, GraphicsApi=%s, AvailableMemory=%llu)", 
                        applicationName,
                        fps,
                        frameTimeInSeconds * 1000.0,
                        renderSize.Width,
                        renderSize.Height,
                        renderSize.UIScale,
                        graphicsDeviceInfo.DeviceName, 
                        SampleGetGraphicsApiLabel(graphicsDeviceInfo.GraphicsApi),
                        graphicsDeviceInfo.AvailableMemory);
    ElemSetWindowTitle(window, temp);
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
