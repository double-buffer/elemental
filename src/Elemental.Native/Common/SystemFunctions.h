#pragma once

#ifdef _WINDOWS
#pragma warning(disable: 5045)
#pragma warning(disable: 4820)
#pragma warning(disable: 4324)
#endif

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

#ifdef _WINDOWS
//HACK: TEMPORARY
#pragma warning(disable: 4100)

#include <io.h>

#define PackedStruct __pragma(pack(push, 1)) struct
#define PackedStructEnd __pragma(pack(pop))

#define popen _popen
#define pclose _pclose
#define wcsdup _wcsdup
#define strdup _strdup
#define mkstemp(value) _mktemp_s(value, MAX_PATH)
#else
#define PackedStruct struct __attribute__((__packed__))
#define PackedStructEnd
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
    const char* libraryExtension = ".dll";
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

#ifdef _MSC_VER // Microsoft compilers

#define GET_ARG_COUNT(...)  INTERNAL_EXPAND_ARGS_PRIVATE(INTERNAL_ARGS_AUGMENTER(__VA_ARGS__))

#define INTERNAL_ARGS_AUGMENTER(...) unused, __VA_ARGS__
#define INTERNAL_EXPAND(x) x
#define INTERNAL_EXPAND_ARGS_PRIVATE(...) INTERNAL_EXPAND(INTERNAL_GET_ARG_COUNT_PRIVATE(__VA_ARGS__, 69, 68, 67, 66, 65, 64, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0))
#define INTERNAL_GET_ARG_COUNT_PRIVATE(_1_, _2_, _3_, _4_, _5_, _6_, _7_, _8_, _9_, _10_, _11_, _12_, _13_, _14_, _15_, _16_, _17_, _18_, _19_, _20_, _21_, _22_, _23_, _24_, _25_, _26_, _27_, _28_, _29_, _30_, _31_, _32_, _33_, _34_, _35_, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, count, ...) count

#else // Non-Microsoft compilers

#define GET_ARG_COUNT(...) INTERNAL_GET_ARG_COUNT_PRIVATE(0, ## __VA_ARGS__, 70, 69, 68, 67, 66, 65, 64, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#define INTERNAL_GET_ARG_COUNT_PRIVATE(_0, _1_, _2_, _3_, _4_, _5_, _6_, _7_, _8_, _9_, _10_, _11_, _12_, _13_, _14_, _15_, _16_, _17_, _18_, _19_, _20_, _21_, _22_, _23_, _24_, _25_, _26_, _27_, _28_, _29_, _30_, _31_, _32_, _33_, _34_, _35_, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, count, ...) count

#endif

//---------------------------------------------------------------------------------------------------------------
// Logging functions
//---------------------------------------------------------------------------------------------------------------
static LogMessageHandlerPtr globalLogMessageHandler = nullptr;

// TODO: Log in output debug

#define LogMessage(type, category, format, ...) \
    { \
        if (!GET_ARG_COUNT(__VA_ARGS__)) { \
            if (globalLogMessageHandler) globalLogMessageHandler(type, category, __LPREFIX(__FUNCTION__), format); \
        } else { \
            wchar_t buffer[256]; \
            int length = swprintf(buffer, sizeof(buffer), format, __VA_ARGS__); \
            if (length >= sizeof(buffer)) { \
                if (globalLogMessageHandler) globalLogMessageHandler(LogMessageType_Warning, category, __LPREFIX(__FUNCTION__), L"Cannot log message"); \
                length = sizeof(buffer) - 1; \
            } \
            buffer[length] = '\0'; \
            if (globalLogMessageHandler) globalLogMessageHandler(type, category, __LPREFIX(__FUNCTION__), buffer); \
        } \
    }

#define LogDebugMessage(category, format, ...) LogMessage(LogMessageType_Debug, category, format, __VA_ARGS__)
#define LogWarningMessage(category, format, ...) LogMessage(LogMessageType_Warning, category, format, __VA_ARGS__)
#define LogErrorMessage(category, format, ...) LogMessage(LogMessageType_Error, category, format, __VA_ARGS__)

//---------------------------------------------------------------------------------------------------------------
// Debug Memory management functions
//---------------------------------------------------------------------------------------------------------------
#ifdef _DEBUG

typedef struct
{
    size_t SizeInBytes;
    wchar_t File[MAX_PATH];
    uint32_t LineNumber;
} SystemAllocation;

static DictionaryStruct* debugAllocations = NULL;

void* SystemAllocateMemory(size_t sizeInBytes, const wchar_t* file, uint32_t lineNumber)
{
    if (debugAllocations == NULL)
    {
        return malloc(sizeInBytes);
    }

    void* pointer = malloc(sizeInBytes);

    SystemAllocation* allocation = (SystemAllocation*)malloc(sizeof(SystemAllocation));
    allocation->SizeInBytes = sizeInBytes;
    wcscpy_s(allocation->File, _countof(allocation->File), file);
    allocation->LineNumber = lineNumber;

    DictionaryAdd(debugAllocations, (size_t)pointer, allocation);

    return pointer;
}

void* SystemAllocateMemoryAndReset(size_t count, size_t size, const wchar_t* file, uint32_t lineNumber)
{
    if (debugAllocations == NULL)
    {
        return calloc(count, size);
    }

    void* pointer = calloc(count, size);
    
    SystemAllocation* allocation = (SystemAllocation*)malloc(sizeof(SystemAllocation));
    allocation->SizeInBytes = count * size;
    wcscpy_s(allocation->File, _countof(allocation->File), file);
    allocation->LineNumber = lineNumber;

    DictionaryAdd(debugAllocations, (size_t)pointer, allocation);
  
    return pointer;
}

void SystemFreeMemory(void* pointer)
{
    if (debugAllocations == NULL)
    {
        free(pointer);
        return;
    }

    if (DictionaryContains(debugAllocations, (size_t)pointer))
    {
        SystemAllocation* allocation = (SystemAllocation*)DictionaryGetEntry(debugAllocations, (size_t)pointer);
        free(allocation);
        DictionaryDelete(debugAllocations, (size_t)pointer);
    }

    free(pointer);
}

void SystemDisplayMemoryLeak(uint64_t key, void* data)
{
    SystemAllocation* value = (SystemAllocation*)data;
    LogWarningMessage(LogMessageCategory_NativeApplication, L"%zu (size in bytes: %zu): %s: %u", (size_t)key, value->SizeInBytes, value->File, value->LineNumber);
}

void SystemInitDebugAllocations()
{
    debugAllocations = DictionaryCreate(64);
}

void SystemCheckAllocations(const wchar_t* description)
{
    //DictionaryPrint(debugAllocations);

    if (debugAllocations->Count > 0)
    {
        LogWarningMessage(LogMessageCategory_NativeApplication, L"Leaked native memory allocations (%s): %zu", description, debugAllocations->Count);
        DictionaryEnumerateEntries(debugAllocations, SystemDisplayMemoryLeak);
    }
    
    DictionaryFree(debugAllocations);
    debugAllocations = NULL;
}

void* operator new(size_t size, const wchar_t* file, uint32_t lineNumber)
{
    return SystemAllocateMemory(size, file, lineNumber);
}

void* operator new[](size_t size, const wchar_t* file, uint32_t lineNumber)
{
    return SystemAllocateMemory(size, file, lineNumber);
}

void operator delete(void* pointer) noexcept
{
    return SystemFreeMemory(pointer);
}

void operator delete[](void* pointer) noexcept
{
    return SystemFreeMemory(pointer);
}

void operator delete(void* pointer, const wchar_t* file, uint32_t lineNumber)
{
    SystemFreeMemory(pointer);
}

void operator delete[](void* pointer, const wchar_t* file, uint32_t lineNumber)
{
    SystemFreeMemory(pointer);
}

#define new new(__LPREFIX(__FILE__), (uint32_t)__LINE__)

#define malloc(size) SystemAllocateMemory(size,__LPREFIX(__FILE__), (uint32_t)__LINE__)
#define calloc(count, size) SystemAllocateMemoryAndReset(count, size, __LPREFIX(__FILE__), (uint32_t)__LINE__)
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

char* SystemConcatStrings(const char* str1, const char* str2) 
{
    size_t len1 = strlen(str1);
    size_t len2 = strlen(str2);

    //char* destination = (char*)SystemAllocateMemory(len1 + len2 + 1, str1, 0);
    char* destination = (char*)malloc(len1 + len2 + 1);

    memcpy(destination, str1, len1);
    memcpy(destination + len1, str2, len2);

    destination[len1 + len2] = '\0';

    return destination;
}

void SystemSplitString(const wchar_t* source, const wchar_t separator, wchar_t** result, uint32_t* resultCount) 
{
    if (!source || !result || !resultCount) 
    {
        return;
    }

    wchar_t* p = (wchar_t*)source;
    uint32_t count = 0;
    while (*p != L'\0') {
        if (*p == separator) {
            ++count;
        }
        ++p;
    }
    ++count;

    *resultCount = count;

    if (result)
    {
        wchar_t* copy = wcsdup(source);
        if (!copy) {
            *result = NULL;
            return;
        }

        p = copy;
        uint32_t index = 0;
        while (*p != L'\0') {
            if (*p == separator) {
                *p = L'\0';
            } else if (index == 0 || *(p - 1) == L'\0') {
                result[index++] = p;
            }
            ++p;
        }
    }
}

const uint8_t* SystemConvertWideCharToUtf8(const wchar_t* source)
{
    size_t requiredSize;
    errno_t error = wcstombs_s(&requiredSize, NULL, 0, source, 0);

    if (error != 0) 
    {
        return NULL;
    }

    uint8_t* destination = (uint8_t*)malloc(requiredSize + 1);
    size_t convertedSize;
    error = wcstombs_s(&convertedSize, (char*)destination, requiredSize, source, requiredSize);

    if (error != 0) 
    {
        free(destination);
        return NULL;
    }

    destination[convertedSize] = '\0';
    return destination;
}

const wchar_t* SystemConvertUtf8ToWideChar(const char* source)
{
    size_t requiredSize;
    errno_t error = mbstowcs_s(&requiredSize, NULL, 0, (char *)source, 0);

    if (error != 0) 
    {
        return NULL;
    }

    //wchar_t* destination = (wchar_t*)SystemAllocateMemory((requiredSize + 1) * sizeof(wchar_t), (const char*)source, 0);
    wchar_t* destination = (wchar_t*)malloc((requiredSize + 1) * sizeof(wchar_t));
    size_t convertedSize;
    error = mbstowcs_s(&convertedSize, destination, requiredSize, (char*)source, requiredSize);

    if (error != 0)
    {
        free(destination);
        return NULL;
    }

    destination[convertedSize] = L'\0';
    return destination;
}

void SystemFreeConvertedString(const wchar_t* value)
{
    free((void*)value);
}

//---------------------------------------------------------------------------------------------------------------
// IO functions
//---------------------------------------------------------------------------------------------------------------

char* SystemGenerateTempFilename() 
{
    char temp[] = "/tmp/tempfileXXXXXX";
    int fd = mkstemp(temp);
    
    if (fd != -1) 
    {
        return strdup(temp);
    }

    return NULL;
}

#ifdef _WINDOWS

std::wstring SystemGetExecutableFolderPath() 
{
    wchar_t path[MAX_PATH];
    GetModuleFileName(NULL, path, MAX_PATH);

    // Find the position of the last backslash character
    wchar_t* last_slash = wcsrchr(path, L'\\');

    if (last_slash != NULL) 
    {
        // Create a string with the folder path
        return std::wstring(path, (size_t)(last_slash - path));
    }
    else 
    {
        // No backslash character found, so return an empty string
        return L"";
    }
}
#endif

void SystemWriteBytesToFile(const char* filename, uint8_t* data, uint32_t dataSizeInBytes) 
{
    FILE *file;
   
    errno_t error = fopen_s(&file, filename, "wb");

    if (error != 0)
    {
        return;
    }

    fwrite(data, sizeof(uint8_t), dataSizeInBytes, file);
    fclose(file);
}

void SystemReadBytesFromFile(const char* filename, uint8_t** data, size_t* dataSizeInBytes)
{
    FILE *file;
   
    errno_t error = fopen_s(&file, filename, "rb");

    if (error != 0)
    {
        *data = NULL;
        *dataSizeInBytes = 0;
    }

    fseek(file, 0, SEEK_END);
    size_t fileSize = (size_t)ftell(file);
    rewind(file);

    uint8_t* outputData = (uint8_t*)malloc(fileSize);

    size_t bytesRead = fread(outputData, sizeof(uint8_t), fileSize, file);
    assert(bytesRead == fileSize);
    fclose(file);

    *data = outputData;
    *dataSizeInBytes = bytesRead;
}

void SystemDeleteFile(const char* filename)
{
   remove(filename); 
}

//---------------------------------------------------------------------------------------------------------------
// Library / process functions
//---------------------------------------------------------------------------------------------------------------

const void* SystemLoadLibrary(const char* libraryName)
{
    char* fullName = SystemConcatStrings(libraryName, libraryExtension);

#ifdef _WINDOWS
    void* library = LoadLibraryA(fullName);
    free(fullName);
    return library;
#else
    char* fullNameUnix = SystemConcatStrings("lib", fullName);
    void* library = dlopen(fullNameUnix, RTLD_LAZY);
    
    free(fullName);
    free(fullNameUnix);
    return library;
#endif
}

void SystemFreeLibrary(const void* library)
{
#ifdef _WINDOWS
    FreeLibrary((HMODULE)library);
#else
    dlclose((void*)library);
#endif
}

const void* SystemGetFunctionExport(const void* library, const char* functionName)
{
#ifdef _WINDOWS
    return GetProcAddress((HMODULE)library, functionName);
#else
    return dlsym((void*)library, functionName);
#endif
}

// TODO: check for errors
bool SystemExecuteProcess(const char* command, char* result) 
{
    char buffer[128];
    result[0] = '\0';

    char* finalCommand = SystemConcatStrings(command, " 2>&1");

    FILE* pipe = popen(finalCommand, "r");
    assert(pipe);

    while (fgets(buffer, sizeof(buffer), pipe) != NULL) 
    {
        char* temp = result;
        temp = SystemConcatStrings(temp, buffer);
        strcpy_s(result, 1024, temp);

        free(temp);
    }

    pclose(pipe);
    free(finalCommand);

    return true;
}

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
