#pragma once

#include "ElementalCommon.h"
#include "SystemLogging.h"
#include "SystemMemory.h"
// TODO: REVIEW HEADERS

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <wchar.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <atomic>

#include "Dictionary.h"

#define PackedStruct struct __attribute__((__packed__))

#ifdef _WINDOWS

#include <io.h>

#define popen _popen
#define pclose _pclose
#define wcsdup _wcsdup
#define strdup _strdup
#define mkstemp(value) _mktemp_s(value, MAX_PATH)
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


#ifdef _DEBUG
void* operator new(size_t size, const wchar_t* file, uint32_t lineNumber);
void* operator new[](size_t size, const wchar_t* file, uint32_t lineNumber);
void operator delete(void* pointer) noexcept;
void operator delete[](void* pointer) noexcept;
void operator delete(void* pointer, const wchar_t* file, uint32_t lineNumber);
void operator delete[](void* pointer, const wchar_t* file, uint32_t lineNumber);

#define new new(L"" __FILE__, (uint32_t)__LINE__)

#define malloc(size) SystemAllocateMemory(size, L"" __FILE__, (uint32_t)__LINE__)
#define calloc(count, size) SystemAllocateMemoryAndReset(count, size, L"" __FILE__, (uint32_t)__LINE__)
//TODO: realloc
#define free(pointer) SystemFreeMemory(pointer)

#ifdef __APPLE__

void AppleMemoryRetain()
{
   printf("Retain\n"); 
}


#endif
#endif

//---------------------------------------------------------------------------------------------------------------
// Foundation structures
//---------------------------------------------------------------------------------------------------------------
template<typename T>
struct Span
{
    Span()
    {
        Pointer = nullptr;
        Length = 0;
    }

    Span(T* pointer, uint32_t length) : Pointer(pointer), Length(length)
    {
    }

    T* Pointer;
    uint32_t Length;

    bool IsEmpty()
    {
        return Length == 0;
    }

    static Span<T> Empty()
    {
        return Span<T>();
    }
};

//---------------------------------------------------------------------------------------------------------------
// String functions
//---------------------------------------------------------------------------------------------------------------

char* SystemConcatStrings(const char* str1, const char* str2); 
void SystemSplitString(const wchar_t* source, const wchar_t separator, wchar_t** result, uint32_t* resultCount);
const uint8_t* SystemConvertWideCharToUtf8(const wchar_t* source);
const wchar_t* SystemConvertUtf8ToWideChar(const char* source);
void SystemFreeConvertedString(const wchar_t* value);

//---------------------------------------------------------------------------------------------------------------
// IO functions
//---------------------------------------------------------------------------------------------------------------

char* SystemGenerateTempFilename();

#ifdef _WINDOWS

std::wstring SystemGetExecutableFolderPath(); 
#endif

void SystemWriteBytesToFile(const char* filename, uint8_t* data, uint32_t dataSizeInBytes); 
void SystemReadBytesFromFile(const char* filename, uint8_t** data, size_t* dataSizeInBytes);
void SystemDeleteFile(const char* filename);

//---------------------------------------------------------------------------------------------------------------
// Library / process functions
//---------------------------------------------------------------------------------------------------------------

const void* SystemLoadLibrary(const char* libraryName);
void SystemFreeLibrary(const void* library);
const void* SystemGetFunctionExport(const void* library, const char* functionName);
bool SystemExecuteProcess(const char* command, char* result);

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
