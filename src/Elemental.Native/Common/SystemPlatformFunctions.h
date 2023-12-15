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

/**
 * Retrieves the current environment of the system platform.
 *
 * @param memoryArena - A pointer to the memory arena used for allocating memory for the environment object.
 * @return A pointer to the SystemPlatformEnvironment object representing the current environment of the system platform.
 */
SystemPlatformEnvironment* SystemPlatformGetEnvironment(MemoryArena* memoryArena);

/**
 * Retrieves the current date and time of the system platform.
 *
 * @param memoryArena - A pointer to the memory arena used for allocating memory for the date and time object.
 * @return A pointer to the SystemPlatformDateTime object representing the current date and time of the system platform.
 */
SystemPlatformDateTime* SystemPlatformGetCurrentDateTime(MemoryArena* memoryArena);

/**
 * Allocates a block of memory of the specified size.
 *
 * @param sizeInBytes The size of the memory block to allocate, in bytes.
 * @return A pointer to the allocated memory block.
 */
void* SystemPlatformAllocateMemory(size_t sizeInBytes);

/**
 * Frees a previously allocated memory block.
 *
 * @param pointer A pointer to the memory block to free.
 */
void SystemPlatformFreeMemory(void* pointer);

/**
 * Clears the contents of a memory block.
 *
 * @param pointer A pointer to the memory block to clear.
 * @param sizeInBytes The size of the memory block to clear, in bytes.
 */
void SystemPlatformClearMemory(void* pointer, size_t sizeInBytes);

/**
 * Copies the contents of one memory block to another.
 *
 * @param destination A pointer to the destination memory block.
 * @param source A pointer to the source memory block.
 * @param sizeInBytes The size of the memory block to copy, in bytes.
 */
void SystemPlatformCopyMemory(void* destination, const void* source, size_t sizeInBytes);

/**
 * Retrieves the path of the current executable.
 *
 * @param memoryArena - A pointer to a MemoryArena struct that will be used for memory allocation.
 * @return A ReadOnlySpan<char> representing the path of the current executable.
 */
ReadOnlySpan<char> SystemPlatformGetExecutablePath(MemoryArena* memoryArena);

/**
 * Retrieves the size of a file in bytes.
 *
 * @param path - A ReadOnlySpan<char> representing the path of the file.
 * @return The size of the file in bytes.
 */
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
