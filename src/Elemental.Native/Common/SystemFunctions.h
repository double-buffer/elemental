#pragma once

#include "SystemMemory.h"

#include <assert.h>

#define PackedStruct struct __attribute__((__packed__))

#ifdef _WINDOWS

#include <io.h>

#define popen _popen
#define pclose _pclose

#else
#define MAX_PATH 256
#ifndef NDEBUG
    #define _DEBUG
#endif
#include <unistd.h>
#include <dlfcn.h>
#define fopen_s(pFile, filename, mode) ((*(pFile))=fopen((filename),(mode)))==NULL
#define errno_t uint64_t
#define DllExport extern "C" __attribute__((visibility("default"))) 
#endif
#ifdef _WINDOWS
    static const char* libraryExtension = ".dll";
#elif __APPLE__
    const char* libraryExtension = ".dylib";
    //HACK
    //#define size_t uint64_t
#else
    const char* libraryExtension = ".so";
#endif

// TODO: Remove AssertIfFailed, only use assert is some specific places
#ifdef _DEBUG
    #define AssertIfFailed(result) assert(!FAILED(result))
#else
    #define AssertIfFailed(result) result
#endif


//---------------------------------------------------------------------------------------------------------------
// Math functions
//---------------------------------------------------------------------------------------------------------------

/**
 * Rounds up the given value to the nearest power of 2.
 *
 * @param value The input value.
 * @return The rounded-up value to the nearest power of 2.
 */
size_t SystemRoundUpToPowerOf2(size_t value);

/**
 * Rounds the given double value to the nearest integer.
 *
 * @param value The input value.
 * @return The rounded value.
 */
double SystemRound(double value);

/**
 * Calculates the absolute value of the given input.
 *
 * @tparam T The type of the input value.
 * @param value The input value.
 * @return The absolute value of the input.
 */
template<typename T>
T SystemAbs(T value);


//---------------------------------------------------------------------------------------------------------------
// String functions
//---------------------------------------------------------------------------------------------------------------

/**
 * Converts a number to a string using the provided memory arena.
 *
 * @tparam T The type of the input value.
 * @param memoryArena The memory arena for string conversion.
 * @param value The input value.
 * @return A read-only span containing the converted string.
 */
template<typename T>
ReadOnlySpan<char> SystemConvertNumberToString(MemoryArena* memoryArena, T value);

/**
 * Converts a double value to a string using the provided memory arena.
 *
 * @param memoryArena The memory arena for string conversion.
 * @param value The input value.
 * @return A read-only span containing the converted string.
 */
ReadOnlySpan<char> SystemConvertFloatToString(MemoryArena* memoryArena, double value);

/**
 * Formats a string using the provided memory arena and format specifier.
 *
 * @param memoryArena The memory arena for string formatting.
 * @param format The format specifier.
 * @param ... Additional arguments for formatting.
 * @return A read-only span containing the formatted string.
 */
ReadOnlySpan<char> SystemFormatString(MemoryArena* memoryArena, ReadOnlySpan<char> format, ...);

/**
 * Formats a string using the provided memory arena, format specifier, and variable arguments.
 *
 * @param memoryArena The memory arena for string formatting.
 * @param format The format specifier.
 * @param arguments The variable arguments for formatting.
 * @return A read-only span containing the formatted string.
 */
ReadOnlySpan<char> SystemFormatString(MemoryArena* memoryArena, ReadOnlySpan<char> format, __builtin_va_list arguments);

/**
 * Splits a string into substrings based on the specified separator.
 *
 * @param memoryArena The memory arena for string splitting.
 * @param source The source string to split.
 * @param separator The character used as a separator.
 * @return A read-only span of read-only spans containing the split substrings.
 */
ReadOnlySpan<ReadOnlySpan<char>> SystemSplitString(MemoryArena* memoryArena, ReadOnlySpan<char> source, char separator);

/**
 * Finds the last index of the specified character in the given string.
 *
 * @param source The source string to search.
 * @param separator The character to search for.
 * @return The last index of the specified character, or -1 if not found.
 */
int64_t SystemLastIndexOf(ReadOnlySpan<char> source, char separator);

/**
 * Converts a UTF-8 encoded string to a wide character (UTF-16) string.
 *
 * @param memoryArena The memory arena for string conversion.
 * @param source The UTF-8 encoded source string.
 * @return A read-only span containing the wide character string.
 */
ReadOnlySpan<wchar_t> SystemConvertUtf8ToWideChar(MemoryArena* memoryArena, ReadOnlySpan<char> source);

/**
 * Converts a wide character (UTF-16) string to UTF-8 encoding.
 *
 * @param memoryArena The memory arena for string conversion.
 * @param source The wide character source string.
 * @return A read-only span containing the UTF-8 encoded string.
 */
ReadOnlySpan<char> SystemConvertWideCharToUtf8(MemoryArena* memoryArena, ReadOnlySpan<wchar_t> source);


//---------------------------------------------------------------------------------------------------------------
// IO functions
//---------------------------------------------------------------------------------------------------------------

/**
 * Generates a temporary filename with the specified prefix.
 *
 * @param memoryArena Pointer to the memory arena for dynamic memory allocation.
 * @param prefix The prefix for the temporary filename.
 * @return A read-only span containing the generated temporary filename.
 */
ReadOnlySpan<char> SystemGenerateTempFilename(MemoryArena* memoryArena, ReadOnlySpan<char> prefix);

/**
 * Retrieves the executable folder path.
 *
 * @param memoryArena Pointer to the memory arena for dynamic memory allocation.
 * @return A read-only span containing the executable folder path.
 */
ReadOnlySpan<char> SystemGetExecutableFolderPath(MemoryArena* memoryArena);

/**
 * Checks if a file exists at the specified path.
 *
 * @param path The path to the file.
 * @return True if the file exists; otherwise, false.
 */
bool SystemFileExists(ReadOnlySpan<char> path);

/**
 * Writes an array of bytes to a file at the specified path.
 *
 * @param path The path to the file.
 * @param data A read-only span containing the data to be written.
 */
void SystemFileWriteBytes(ReadOnlySpan<char> path, ReadOnlySpan<uint8_t> data);

/**
 * Reads the contents of a file into a span of bytes.
 *
 * @param memoryArena Pointer to the memory arena for dynamic memory allocation.
 * @param path The path to the file.
 * @return A span containing the read bytes.
 */
Span<uint8_t> SystemFileReadBytes(MemoryArena* memoryArena, ReadOnlySpan<char> path);

/**
 * Deletes the file at the specified path.
 *
 * @param path The path to the file to be deleted.
 */
void SystemFileDelete(ReadOnlySpan<char> path);


//---------------------------------------------------------------------------------------------------------------
// Library / process functions
//---------------------------------------------------------------------------------------------------------------

ReadOnlySpan<char> SystemExecuteProcess(MemoryArena* memoryArena, ReadOnlySpan<char> command);

// TODO: Old CODE to remove
const void* SystemLoadLibrary(const char* libraryName);
void SystemFreeLibrary(const void* library);
const void* SystemGetFunctionExport(const void* library, const char* functionName);


//---------------------------------------------------------------------------------------------------------------
// Threading functions
//---------------------------------------------------------------------------------------------------------------

// TODO: windows functions
#ifndef _WINDOWS
struct SystemThread
{
    pthread_t ThreadHandle;
};

typedef void* (*SystemThreadFunction)(void* parameters);

SystemThread SystemCreateThread(SystemThreadFunction threadFunction)
{
    SystemThread result = {};

    auto resultCode = pthread_create(&result.ThreadHandle, nullptr, threadFunction, nullptr);

    if (resultCode != 0)
    {
        printf("error: Create Thread: cannot create system thread.\n");
        return {};
    }

    return result;
}
#endif
