#pragma once

#include "SystemMemory.h"

// TODO: REVIEW HEADERS
#include <stdint.h>
#include <stdlib.h>
#include <wchar.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

// TODO: To remove -> Temporary
#include <string>

#define PackedStruct struct __attribute__((__packed__))

#ifdef _WINDOWS

#include <io.h>

#define popen _popen
#define pclose _pclose

#define mkstemp(value) _mktemp_s(value, strlen(value) + 1)
#else
#define MAX_PATH 256
#ifndef NDEBUG
    #define _DEBUG
#endif
#include <unistd.h>
#include <dlfcn.h>
#define fopen_s(pFile, filename, mode) ((*(pFile))=fopen((filename),(mode)))==NULL
#define wcstombs_s(returnValue, destination, size, source, maxSize) ((*(returnValue))=wcstombs((destination), (source), (size)))==-1
#define mbstowcs_s(returnValue, destination, size, source, maxSize) ((*(returnValue))=mbstowcs((destination), (source), (size)))==-1
#define strcpy_s(destination, size, source) strcpy(destination, source)
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

#ifdef _DEBUG
    #define AssertIfFailed(result) assert(!FAILED(result))
#else
    #define AssertIfFailed(result) result
#endif

//---------------------------------------------------------------------------------------------------------------
// String functions
//---------------------------------------------------------------------------------------------------------------

// TODO: It would be great to have an opaque string type that can be treated as UTF-8 or Unicode based on settings
// passed to the native application service

template<typename T>
ReadOnlySpan<ReadOnlySpan<T>> SystemSplitString(MemoryArena* memoryArena, ReadOnlySpan<T> source, T separator);

ReadOnlySpan<char> SystemConvertWideCharToUtf8(MemoryArena* memoryArena, ReadOnlySpan<wchar_t> source);
ReadOnlySpan<wchar_t> SystemConvertUtf8ToWideChar(MemoryArena* memoryArena, ReadOnlySpan<char> source);


// TODO: OLD CODE to remove
void SystemFreeConvertedString(const wchar_t* value);

//---------------------------------------------------------------------------------------------------------------
// IO functions
//---------------------------------------------------------------------------------------------------------------

ReadOnlySpan<char> SystemGenerateTempFilename(MemoryArena* memoryArena); 

#ifdef _WINDOWS

std::wstring SystemGetExecutableFolderPath(); 
#endif

void SystemWriteBytesToFile(const char* filename, uint8_t* data, uint32_t dataSizeInBytes); 
void SystemReadBytesFromFile(const char* filename, uint8_t** data, size_t* dataSizeInBytes);
void SystemDeleteFile(const char* filename);

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
