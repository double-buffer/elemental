#pragma once

// TODO: Remove c++

#ifdef _WINDOWS
#define PackedStruct __pragma(pack(push, 1)) struct
#define PackedStructEnd __pragma(pack(pop))
#else
#define PackedStruct struct __attribute__((__packed__))
#define PackedStructEnd
#endif

#if _WINDOWS
uint8_t* SystemConvertWStringToUtf8(const std::wstring &source)
{
    char *destination = new char[source.length() + 1];
    destination[source.length()] = '\0';
    WideCharToMultiByte(65001, 0, source.c_str(), -1, destination, (int)source.length(), NULL, NULL);

    return (uint8_t*)destination;
}

std::wstring SystemConvertUtf8ToWString(uint8_t* source)
{
    auto stringLength = std::string((char*)source).length();
    std::wstring destination;
    destination.resize(stringLength + 1);
    MultiByteToWideChar(CP_UTF8, 0, (char*)source, -1, (wchar_t*)destination.c_str(), (int)(stringLength + 1));

	return destination;
}
std::wstring SystemGenerateTempFilename() 
{
    wchar_t temp[MAX_PATH];

    // TODO: Get users temp directory

    DWORD dwRetVal = GetTempFileName(L".", L"tempfile", 0, temp);
    if (dwRetVal != 0) {
        return std::wstring(temp);
    }
    return L"";
}
#else
uint8_t* SystemConvertWStringToUtf8(const std::wstring &source)
{
    iconv_t cd = iconv_open("UTF-8", "WCHAR_T");

    char* output_buffer = new char[source.length() * 4];
    char* output_ptr = output_buffer;
    size_t output_len = source.length() * 4;

    char* input_ptr = reinterpret_cast<char*>((char*)source.c_str());
    size_t input_len = source.length() * sizeof(wchar_t);

    int result = iconv(cd, &input_ptr, &input_len, &output_ptr, &output_len);

    return (uint8_t*)output_buffer;
}

// TODO: This seems to be the thing to do, convert the other functions
std::wstring SystemConvertUtf8ToWString(const uint8_t* source)
{
    iconv_t cd = iconv_open("WCHAR_T", "UTF-8");
    
    auto inputString = std::string((char*)source);
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    auto outputString = converter.from_bytes(inputString);
    
    return outputString;
}

std::string SystemGenerateTempFilename() 
{
    char temp[] = "/tmp/tempfileXXXXXX";
    int fd = mkstemp(temp);
    if (fd != -1) {
        close(fd);
        return std::string(temp);
    }
    return "";
}
#endif
void* SystemLoadLibrary(const std::string libraryName)
{
#ifdef _WINDOWS
    const std::string libraryExtension = ".dll";
#elif __APPLE__
    const std::string libraryExtension = ".dylib";
#else
    const std::string libraryExtension = ".so";
#endif

#ifdef _WINDOWS
    return LoadLibraryA((libraryName + libraryExtension).c_str());
#else
    return dlopen(("lib" + libraryName + libraryExtension).c_str(), RTLD_LAZY);
#endif
}

void SystemFreeLibrary(void* library)
{
#ifdef _WINDOWS
    FreeLibrary((HMODULE)library);
#else
    dlclose(library);
#endif
}

void* SystemGetFunctionExport(void* library, std::string functionName)
{
#ifdef _WINDOWS
    return GetProcAddress((HMODULE)library, functionName.c_str());
#else
    return dlsym(library, functionName.c_str());
#endif
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

void SystemWriteBytesToFile(const std::string& filename, Span<uint8_t> data) 
{
    FILE* outFile = fopen(filename.c_str(), "wb");
    fwrite(data.Pointer, sizeof(uint8_t), data.Length, outFile);
    fclose(outFile);
}

Span<uint8_t> SystemReadBytesFromFile(const std::string& filename)
{
    FILE* inFile = fopen(filename.c_str(), "rb");

    if (!inFile) 
    {
        throw std::runtime_error("Failed to open file: " + filename);
    }

    std::fseek(inFile, 0, SEEK_END);
    std::size_t fileSize = std::ftell(inFile);
    std::rewind(inFile);

    auto outputData = new uint8_t[fileSize];
    std::size_t bytesRead = std::fread(outputData, sizeof(uint8_t), fileSize, inFile);

    assert(bytesRead == fileSize);
    std::fclose(inFile);

    return Span<uint8_t>(outputData, fileSize);
}

void SystemDeleteFile(const std::string& filename)
{
   remove(filename.c_str()); 
}

std::string SystemExecuteProcess(const std::string cmd) 
{
    char buffer[128];
    std::string result = "";

    FILE* pipe = popen((cmd + " 2>&1").c_str(), "r");
    assert(pipe);

    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) 
    {
        result += buffer;
    }

    pclose(pipe);
    return result;
}

// TODO: Write system allocate/free functions so we can track if we have memory leak in the interop layer

void* SystemLoadLibrary(const std::string libraryName)
{
#ifdef _WINDOWS
    const std::string libraryExtension = ".dll";
#elif __APPLE__
    const std::string libraryExtension = ".dylib";
#else
    const std::string libraryExtension = ".so";
#endif

#ifdef _WINDOWS
    return LoadLibraryA((libraryName + libraryExtension).c_str());
#else
    return dlopen(("lib" + libraryName + libraryExtension).c_str(), RTLD_LAZY);
#endif
}

void SystemFreeLibrary(void* library)
{
#ifdef _WINDOWS
    FreeLibrary((HMODULE)library);
#else
    dlclose(library);
#endif
}

void* SystemGetFunctionExport(void* library, std::string functionName)
{
#ifdef _WINDOWS
    return GetProcAddress((HMODULE)library, functionName.c_str());
#else
    return dlsym(library, functionName.c_str());
#endif
}

#ifdef _WINDOWS
std::wstring ConvertUtf8ToWString(unsigned char* source)
{
    auto stringLength = std::string((char*)source).length();
    std::wstring destination;
    destination.resize(stringLength + 1);
    MultiByteToWideChar(CP_UTF8, 0, (char*)source, -1, (wchar_t*)destination.c_str(), (int)(stringLength + 1));

	return destination;
}

unsigned char* ConvertWStringToUtf8(const std::wstring &source)
{
    char *destination = new char[source.length() + 1];
    destination[source.length()] = '\0';
    WideCharToMultiByte(CP_ACP, 0, source.c_str(), -1, destination, (int)source.length(), NULL, NULL);

    return (unsigned char*)destination;
}

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