#pragma once

// TODO: Find a solution here
#pragma warning(disable :5045)


//HACK: TEMPORARY
#pragma warning(disable: 4100)

// TODO: Remove c++
#include <string>
#include <vector>

#ifdef _WINDOWS
#define PackedStruct __pragma(pack(push, 1)) struct
#define PackedStructEnd __pragma(pack(pop))

#define popen _popen
#define pclose _pclose
#else
#define PackedStruct struct __attribute__((__packed__))
#define PackedStructEnd
#endif
#ifdef _WINDOWS
    const char* libraryExtension = ".dll";
#elif __APPLE__
    const char* libraryExtension = ".dylib";
#else
    const char* libraryExtension = ".so";
#endif

//---------------------------------------------------------------------------------------------------------------
// Memory management functions
//---------------------------------------------------------------------------------------------------------------
// TODO: Write system allocate/free functions so we can track if we have memory leak in the interop layer

uint32_t globalAllocationCount = 0;

void* SystemAllocateMemory(uint64_t sizeInBytes)
{
    void* memory = malloc(sizeInBytes);
    globalAllocationCount++;
    return memory;
}

void SystemFreeMemory(void* memory)
{
    free(memory);
}

//---------------------------------------------------------------------------------------------------------------
// String functions
//---------------------------------------------------------------------------------------------------------------

void SystemConcatStrings(char* dest, const char* str1, const char* str2) 
{
    // get the lengths of the strings
    size_t len1 = strlen(str1);
    size_t len2 = strlen(str2);

    // copy the first string into the buffer
    memcpy(dest, str1, len1);

    // copy the second string into the buffer, starting at the end of the first string
    memcpy(dest + len1, str2, len2);

    // null-terminate the concatenated string
    dest[len1 + len2] = '\0';
}

std::vector<std::wstring> SystemSplitString(std::wstring w, std::wstring tokenizerStr) 
{
    std::vector<std::wstring> result;
    long tokeninzerLength = tokenizerStr.length();
    long position = 0;
    long findIndex = w.find(tokenizerStr, position);

    while (findIndex != -1)
    {
        std::wstring str = w.substr(position, findIndex - position);
        result.push_back(str);
        position = findIndex + tokeninzerLength;
        findIndex = w.find(tokenizerStr, position);
    }

    result.push_back(w.substr(position, w.length() - position));

    return result;
}

const uint8_t* SystemConvertWideCharToUtf8(const wchar_t* source)
{
    // first, determine the required buffer size
    size_t dest_size = 0;
    errno_t err = wcstombs_s(&dest_size, NULL, 0, source, 0);
    if (err != 0 || dest_size == 0) {
        // conversion failed
        return NULL;
    }

    // allocate a buffer for the encoded string
    char* dest = (char*)malloc(dest_size + 1); // add 1 for null terminator
    if (dest == NULL) {
        // memory allocation failed
        return NULL;
    }

    // encode the string
    size_t converted_chars = 0;
    err = wcstombs_s(&converted_chars, dest, dest_size + 1, source, dest_size);
    if (err != 0 || converted_chars == 0) {
        // conversion failed
        free(dest);
        return NULL;
    }

    // null-terminate the string
    dest[converted_chars] = '\0';

    return (uint8_t*)dest;
}

const wchar_t* SystemConvertUtf8ToWideChar(const uint8_t* source)
{
    // first, determine the required buffer size
    size_t dest_size = 0;
    errno_t err = mbstowcs_s(&dest_size, NULL, 0, (char*)source, 0);
    if (err != 0 || dest_size == 0) {
        // conversion failed
        return NULL;
    }

    // allocate a buffer for the converted string
    wchar_t* dest = (wchar_t*)malloc((dest_size + 1) * sizeof(wchar_t)); // add 1 for null terminator
    if (dest == NULL) {
        // memory allocation failed
        return NULL;
    }

    // convert the string
    size_t converted_chars = 0;
    err = mbstowcs_s(&converted_chars, dest, dest_size + 1, (char*)source, dest_size);
    if (err != 0 || converted_chars == 0) {
        // conversion failed
        free(dest);
        return NULL;
    }

    // null-terminate the string
    dest[converted_chars] = L'\0';

    return dest;
}

//---------------------------------------------------------------------------------------------------------------
// IO functions
//---------------------------------------------------------------------------------------------------------------

#if _WINDOWS
const wchar_t* SystemGenerateTempFilename() 
{
    // TODO: HACK
    wchar_t* temp = (wchar_t*)malloc(MAX_PATH * sizeof(wchar_t));

    // TODO: Get users temp directory

    DWORD dwRetVal = GetTempFileName(L".", L"tempfile", 0, temp);
    if (dwRetVal != 0) {
        return temp;
    }
    return L"";
}
#else
const wchar_t* SystemGenerateTempFilename() 
{
    char temp[] = "/tmp/tempfileXXXXXX";
    int fd = mkstemp(temp);
    if (fd != -1) {
        close(fd);
        return temp;
    }
    return "";
}
#endif

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
        return std::wstring(path, last_slash - path);
    }
    else 
    {
        // No backslash character found, so return an empty string
        return L"";
    }
}
#endif

void SystemWriteBytesToFile(const std::string& filename, uint8_t* data, uint32_t dataSizeInBytes) 
{
    FILE* file = NULL;
    errno_t result = fopen_s(&file, filename.c_str(), "wb");

    if (result != 0 || file == NULL)
    {
        return;
    }

    fwrite(data, sizeof(uint8_t), dataSizeInBytes, file);
    fclose(file);
}

void SystemReadBytesFromFile(const std::string& filename, uint8_t** data, uint32_t* dataSizeInBytes)
{
    FILE* file = NULL;
    errno_t result = fopen_s(&file, filename.c_str(), "rb");

    if (result != 0 || file == NULL)
    {
        *data = NULL;
        *dataSizeInBytes = 0;
    }

    std::fseek(file, 0, SEEK_END);
    std::size_t fileSize = std::ftell(file);
    std::rewind(file);

    auto outputData = new uint8_t[fileSize];
    std::size_t bytesRead = std::fread(outputData, sizeof(uint8_t), fileSize, file);

    assert(bytesRead == fileSize);
    std::fclose(file);

    *data = outputData;
    *dataSizeInBytes = fileSize;
}

void SystemDeleteFile(const std::string& filename)
{
   remove(filename.c_str()); 
}

//---------------------------------------------------------------------------------------------------------------
// Library / process functions
//---------------------------------------------------------------------------------------------------------------

const void* SystemLoadLibrary(const char* libraryName)
{
    char fullName[MAX_PATH];
    SystemConcatStrings(fullName, libraryName, libraryExtension);

#ifdef _WINDOWS
    return LoadLibraryA(fullName);
#else
    char* fullNameUnix[MAX_PATH];
    return dlopen(SystemContacStrings(fullNameUnix, "lib", fullName), RTLD_LAZY);
#endif
}

void SystemFreeLibrary(const void* library)
{
#ifdef _WINDOWS
    FreeLibrary((HMODULE)library);
#else
    dlclose(library);
#endif
}

const void* SystemGetFunctionExport(const void* library, const char* functionName)
{
#ifdef _WINDOWS
    return GetProcAddress((HMODULE)library, functionName);
#else
    return dlsym(library, functionName);
#endif
}

std::string SystemExecuteProcess(const std::string cmd) 
{
    char buffer[128];
    std::string result = "";

    FILE* pipe = popen((cmd + " 2>&1").c_str(), "r");
    assert(pipe);

    while (fgets(buffer, sizeof(buffer), pipe) != NULL) 
    {
        result += buffer;
    }

    pclose(pipe);
    return result;
}