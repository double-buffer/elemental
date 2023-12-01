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
size_t SystemPlatformFileGetSizeInBytes(ReadOnlySpan<char> path);
bool SystemPlatformFileExists(ReadOnlySpan<char> path);
void SystemPlatformFileWriteBytes(ReadOnlySpan<char> path, ReadOnlySpan<uint8_t> data);
void SystemPlatformFileReadBytes(ReadOnlySpan<char> path, Span<uint8_t> data);
void SystemPlatformFileDelete(ReadOnlySpan<char> path);

ReadOnlySpan<char> SystemPlatformExecuteProcess(MemoryArena* memoryArena, ReadOnlySpan<char> command);
void* SystemPlatformLoadLibrary(ReadOnlySpan<char> libraryName);
void SystemPlatformFreeLibrary(const void* library);
void* SystemPlatformGetFunctionExport(const void* library, ReadOnlySpan<char> functionName);

void* SystemPlatformCreateThread(void* threadFunction, void* parameters);
void SystemPlatformFreeThread(void* thread);
