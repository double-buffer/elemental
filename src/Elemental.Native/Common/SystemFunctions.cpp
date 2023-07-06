#include "SystemMemory.h"

#ifdef _DEBUG
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
#endif

#include "SystemFunctions.h"

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
                result[index++] = p;
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
    errno_t error = mbstowcs_s(&requiredSize, NULL, 0, (char*)source, 0);

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
    return (void*)GetProcAddress((HMODULE)library, functionName);
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
