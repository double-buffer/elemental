#include "SystemFunctions.h"

//---------------------------------------------------------------------------------------------------------------
// String functions
//---------------------------------------------------------------------------------------------------------------

template<typename T>
ReadOnlySpan<ReadOnlySpan<T>> SystemSplitString(MemoryArena* memoryArena, ReadOnlySpan<T> source, T separator) 
{
   auto count = 1;

    for (size_t i = 0; i < source.Length; i++)
    {
        if (source[i] == separator)
        {
            count++;
        }
    }

    auto result = SystemPushArray<ReadOnlySpan<T>>(memoryArena, count);
    auto currentIndex = 0;

    for (int i = 0; i < count; i++)
    {
        auto separatorIndex = source.Length;

        for (size_t j = currentIndex; j < source.Length; j++)
        {
            if (source[j] == separator)
            {
                separatorIndex = j;
                break;
            }
        }

        auto resultString = SystemPushArrayZero<T>(memoryArena, separatorIndex - currentIndex); 
        source.Slice(currentIndex, separatorIndex - currentIndex).CopyTo(resultString);
        result[i] = resultString;

        currentIndex = separatorIndex + 1;
    }

    return result;
}

ReadOnlySpan<char> SystemConvertWideCharToUtf8(MemoryArena* memoryArena, ReadOnlySpan<wchar_t> source)
{
    auto destination = SystemPushArrayZero<char>(memoryArena, source.Length);
    wcstombs_s(nullptr, destination.Pointer, destination.Length + 1, source.Pointer, source.Length); // TODO: System call to review

    return destination;
}

ReadOnlySpan<wchar_t> SystemConvertUtf8ToWideChar(MemoryArena* memoryArena, ReadOnlySpan<char> source)
{
    auto destination = SystemPushArrayZero<wchar_t>(memoryArena, source.Length);
    mbstowcs_s(nullptr, destination.Pointer, destination.Length + 1, (char*)source.Pointer, source.Length); // TODO: System call to review

    return destination;
}

// TODO: OLD CODE to remove
void SystemFreeConvertedString(const wchar_t* value)
{
    free((void*)value);
}

//---------------------------------------------------------------------------------------------------------------
// IO functions
//---------------------------------------------------------------------------------------------------------------

ReadOnlySpan<char> SystemGenerateTempFilename(MemoryArena* memoryArena) 
{
    auto templateName = (ReadOnlySpan<char>)"/tmp/tempfileXXXXXX";
    auto result = SystemPushArrayZero<char>(memoryArena, templateName.Length);
    templateName.CopyTo(result);

    mkstemp((char*)result.Pointer); // TODO: System call to review

    return result;
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

ReadOnlySpan<char> SystemExecuteProcess(MemoryArena* memoryArena, ReadOnlySpan<char> command)
{
    auto result = SystemPushArrayZero<char>(memoryArena, 2048);

    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto buffer = SystemPushArrayZero<char>(stackMemoryArena, 128);
    auto finalCommand = SystemConcatBuffers<char>(stackMemoryArena, command, " 2>&1");

    auto pipe = popen(finalCommand.Pointer, "r");
    assert(pipe);

    auto currentLength = 0;

    while (fgets(buffer.Pointer, buffer.Length, pipe) != nullptr) 
    {
        auto length = strlen(buffer.Pointer);
        buffer.Slice(0, length).CopyTo(result.Slice(currentLength));

        currentLength += length;
    }

    pclose(pipe);

    return result;
}

// TODO: To remove old code
const void* SystemLoadLibrary(const char* libraryName)
{
    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto fullName = SystemConcatBuffers<char>(stackMemoryArena, libraryName, libraryExtension);

#ifdef _WINDOWS
    void* library = LoadLibraryA(fullName.Pointer);
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
