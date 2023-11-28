#pragma once

#include "SystemMemory.h"

struct SystemPlatformEnvironment
{
    char PathSeparator;
};

struct SystemPlatformDateTime 
{
    int32_t Year;
    int32_t Month;
    int32_t Day;
    int32_t Hour;
    int32_t Minute;
    int32_t Second;
};

SystemPlatformEnvironment* SystemPlatformGetEnvironment(MemoryArena* memoryArena);
SystemPlatformDateTime* SystemPlatformGetCurrentDateTime(MemoryArena* memoryArena);

void* SystemPlatformAllocateMemory(size_t sizeInBytes);
void SystemPlatformFreeMemory(void* pointer);
void SystemPlatformClearMemory(void* pointer, size_t sizeInBytes);
void SystemPlatformCopyMemory(void* destination, const void* source, size_t sizeInBytes);

ReadOnlySpan<char> SystemPlatformGetExecutablePath(MemoryArena* memoryArena);
bool SystemPlatformFileExists(ReadOnlySpan<char> path);
