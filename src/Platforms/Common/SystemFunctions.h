#pragma once

#pragma warning(disable: 5045)
#pragma warning(disable: 4820)
#pragma warning(disable: 4324)

// TODO: REVIEW HEADERS

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <wchar.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

//HACK: TEMPORARY
#pragma warning(disable: 4100)

#ifdef _WINDOWS
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
#define strcpy_s(destination, source) strcpy(destination, source)
#define errno_t uint64_t
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

//---------------------------------------------------------------------------------------------------------------
// Debug Memory management functions
//---------------------------------------------------------------------------------------------------------------
#ifdef _DEBUG
#include "Dictionary.h"

struct SystemAllocation
{
    size_t SizeInBytes;
    char File[MAX_PATH];
    uint32_t LineNumber;
};

static struct DictionaryStruct* debugAllocations = NULL;

void* SystemAllocateMemory(size_t sizeInBytes, const char* file, uint32_t lineNumber)
{
    if (debugAllocations == NULL)
    {
        return malloc(sizeInBytes);
    }

    void* pointer = malloc(sizeInBytes);

    struct SystemAllocation* allocation = (struct SystemAllocation*)malloc(sizeof(struct SystemAllocation));
    allocation->SizeInBytes = sizeInBytes;
    strcpy_s(allocation->File, file);
    allocation->LineNumber = lineNumber;

    DictionaryAdd(debugAllocations, (size_t)pointer, allocation);

    return pointer;
}

void* SystemAllocateMemoryAndReset(size_t count, size_t size, const char* file, uint32_t lineNumber)
{
    if (debugAllocations == NULL)
    {
        return calloc(count, size);
    }

    void* pointer = calloc(count, size);
    
    struct SystemAllocation* allocation = (struct SystemAllocation*)malloc(sizeof(struct SystemAllocation));
    allocation->SizeInBytes = count * size;
    strcpy_s(allocation->File, file);
    allocation->LineNumber = lineNumber;

    DictionaryAdd(debugAllocations, (size_t)pointer, allocation);
  
    return pointer;
}

void SystemFreeMemory(void* pointer, const char* file, uint32_t lineNumber)
{
    if (debugAllocations == NULL)
    {
        free(pointer);
        return;
    }

    if (DictionaryContains(debugAllocations, (size_t)pointer))
    {
        struct SystemAllocation* allocation = (struct SystemAllocation*)DictionaryGetEntry(debugAllocations, (size_t)pointer);
        free(allocation);
        DictionaryDelete(debugAllocations, (size_t)pointer);
    }

    free(pointer);
}

void SystemDisplayMemoryLeak(uint64_t key, void* data)
{
    struct SystemAllocation* value = (struct SystemAllocation*)data;
    printf("%zu (size in bytes: %zu): %s: %u\n", (size_t)key, value->SizeInBytes, value->File, value->LineNumber);
}

void SystemInitDebugAllocations()
{
    debugAllocations = DictionaryCreate(64);
}

void SystemCheckAllocations(const char* description)
{
    DictionaryPrint(debugAllocations);

    if (debugAllocations->Count > 0)
    {
        printf("WARNING: Leaked native memory allocations (%s): %zu\n", description, debugAllocations->Count);
        DictionaryEnumerateEntries(debugAllocations, SystemDisplayMemoryLeak);
    }
    
    DictionaryFree(debugAllocations);
    debugAllocations = NULL;
}

#define malloc(size) SystemAllocateMemory(size, __FILE__, (uint32_t)__LINE__)
#define calloc(count, size) SystemAllocateMemoryAndReset(count, size, __FILE__, (uint32_t)__LINE__)
#define free(pointer) SystemFreeMemory(pointer, __FILE__, (uint32_t)__LINE__)
#endif

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

const wchar_t* SystemConvertUtf8ToWideChar(const uint8_t* source)
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
        strcpy_s(result, temp);

        free(temp);
    }

    pclose(pipe);
    free(finalCommand);

    return true;
}