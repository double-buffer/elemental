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
SystemPlatformEnvironment* SystemPlatformGetEnvironment(MemoryArena memoryArena);

/**
 * Retrieves the current date and time of the system platform.
 *
 * @param memoryArena - A pointer to the memory arena used for allocating memory for the date and time object.
 * @return A pointer to the SystemPlatformDateTime object representing the current date and time of the system platform.
 */
SystemPlatformDateTime* SystemPlatformGetCurrentDateTime(MemoryArena memoryArena);

/**
 * Retrieves the size of the system's memory page.
 * 
 * This function is used to obtain the size of a single page of memory as used by the system's memory management. 
 * The page size is a fundamental property in memory management, as it determines the granularity of memory allocation 
 * and management operations.
 *
 * @return The size of a memory page in bytes.
 */
size_t SystemPlatformGetPageSize();

/**
 * Reserves a block of memory.
 * 
 * This function reserves a region of memory of the specified size. The reserved memory is not committed (i.e., physical 
 * storage has not been allocated). This is typically used in systems to reserve a large block of address space and commit 
 * portions of it as needed.
 *
 * @param sizeInBytes The size of the memory to reserve in bytes.
 * @return A pointer to the beginning of the reserved memory block.
 */
void* SystemPlatformReserveMemory(size_t sizeInBytes);

/**
 * Frees a previously reserved block of memory.
 * 
 * This function releases a previously reserved block of memory, making it available for other uses. The specified memory 
 * block should have been reserved using SystemPlatformReserveMemory.
 *
 * @param pointer A pointer to the start of the memory block to be freed.
 * @param sizeInBytes The size of the memory block in bytes.
 */
void SystemPlatformFreeMemory(void* pointer, size_t sizeInBytes);

/**
 * Commits a block of reserved memory.
 * 
 * After reserving memory using SystemPlatformReserveMemory, this function is used to commit a portion (or all) of that 
 * memory. Committing memory allocates physical storage (RAM or disk) for that memory region.
 *
 * @param pointer A pointer to the start of the memory block to be committed.
 * @param sizeInBytes The size of the memory block to commit in bytes.
 */
void SystemPlatformCommitMemory(void* pointer, size_t sizeInBytes);

/**
 * Decomits a previously committed block of memory.
 * 
 * This function is used to decommit a portion of memory that was previously committed using SystemPlatformCommitMemory. 
 * This effectively frees up the physical storage associated with the memory, while keeping the memory region reserved.
 *
 * @param pointer A pointer to the start of the memory block to decommit.
 * @param sizeInBytes The size of the memory block to decommit in bytes.
 */
void SystemPlatformDecommitMemory(void* pointer, size_t sizeInBytes);

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
ReadOnlySpan<char> SystemPlatformGetExecutablePath(MemoryArena memoryArena);

/**
 * Retrieves the size of a file in bytes.
 *
 * @param path - A ReadOnlySpan<char> representing the path of the file.
 * @return The size of the file in bytes.
 */
size_t SystemPlatformFileGetSizeInBytes(ReadOnlySpan<char> path);

/**
 * Checks if a file exists.
 *
 * @param path A ReadOnlySpan<char> representing the path of the file to check.
 * @return True if the file exists, false otherwise.
 */
bool SystemPlatformFileExists(ReadOnlySpan<char> path);

/**
 * Writes bytes to a file.
 *
 * @param path A ReadOnlySpan<char> representing the path of the file where bytes are to be written.
 * @param data A ReadOnlySpan<uint8_t> containing the data to be written to the file.
 */
void SystemPlatformFileWriteBytes(ReadOnlySpan<char> path, ReadOnlySpan<uint8_t> data);

/**
 * Reads bytes from a file.
 *
 * @param path A ReadOnlySpan<char> representing the path of the file from which bytes are to be read.
 * @param data A Span<uint8_t> where the read data will be stored.
 */
void SystemPlatformFileReadBytes(ReadOnlySpan<char> path, Span<uint8_t> data);

/**
 * Deletes a file.
 *
 * @param path A ReadOnlySpan<char> representing the path of the file to be deleted.
 */
void SystemPlatformFileDelete(ReadOnlySpan<char> path);

/**
 * Executes a process based on a command.
 *
 * @param memoryArena A pointer to a MemoryArena struct used for memory allocation.
 * @param command A ReadOnlySpan<char> representing the command to execute.
 * @return A ReadOnlySpan<char> containing the output of the executed command.
 */
ReadOnlySpan<char> SystemPlatformExecuteProcess(MemoryArena memoryArena, ReadOnlySpan<char> command);

/**
 * Loads a dynamic library.
 *
 * @param libraryName A ReadOnlySpan<char> representing the name of the library to load.
 * @return A pointer to the loaded library.
 */
void* SystemPlatformLoadLibrary(ReadOnlySpan<char> libraryName);

/**
 * Frees a loaded dynamic library.
 *
 * @param library A pointer to the library to free.
 */
void SystemPlatformFreeLibrary(const void* library);

/**
 * Retrieves a function export from a loaded library.
 *
 * @param library A pointer to the loaded library.
 * @param functionName A ReadOnlySpan<char> representing the name of the function to retrieve.
 * @return A pointer to the retrieved function.
 */
void* SystemPlatformGetFunctionExport(const void* library, ReadOnlySpan<char> functionName);

/**
 * Creates a new thread.
 *
 * @param threadFunction A pointer to the function that the thread will execute.
 * @param parameters A pointer to the parameters to be passed to the thread function.
 * @return A pointer to the created thread.
 */
void* SystemPlatformCreateThread(void* threadFunction, void* parameters);

/**
 * Waits for a thread to finish execution.
 *
 * @param thread A pointer to the thread to wait for.
 */
void SystemPlatformWaitThread(void* thread);

/**
 * Yields execution of the current thread to another thread.
 *
 * This function allows the current thread to offer its remaining execution time slice to another thread that is ready to run on the same processor.
 * It's a beneficial approach in a multi-threaded environment to optimize resource usage, especially useful when the current thread has no immediate work to perform.
 * By yielding, the thread can help reduce CPU usage, avoiding unnecessary occupation of processor time when it is essentially idle or in a waiting state.
 */
void SystemPlatformYieldThread();

/**
 * Frees a thread.
 *
 * @param thread A pointer to the thread to free.
 */
void SystemPlatformFreeThread(void* thread);
