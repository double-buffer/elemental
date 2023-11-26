#include "SystemFunctions.h"

size_t SystemRoundUpToPowerOf2(size_t value) 
{
    size_t result = 1;

    while (result < value) 
    {
        result <<= 1;
    }

    return result;
}

//---------------------------------------------------------------------------------------------------------------
// String functions
//---------------------------------------------------------------------------------------------------------------

ReadOnlySpan<ReadOnlySpan<char>> SystemSplitString(MemoryArena* memoryArena, ReadOnlySpan<char> source, char separator) 
{
   auto count = 1;

    for (size_t i = 0; i < source.Length; i++)
    {
        if (source[i] == separator)
        {
            count++;
        }
    }

    auto result = SystemPushArray<ReadOnlySpan<char>>(memoryArena, count);
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

        auto resultString = SystemPushArrayZero<char>(memoryArena, separatorIndex - currentIndex);
        SystemCopyBuffer(resultString, source.Slice(currentIndex, separatorIndex - currentIndex));

        result[i] = resultString;

        currentIndex = separatorIndex + 1;
    }

    return result;
}

ReadOnlySpan<wchar_t> SystemConvertUtf8ToWideChar(MemoryArena* memoryArena, ReadOnlySpan<char> source)
{
    size_t size = 0;
    auto sourceSpan = source;

    while (sourceSpan[0] != '\0')
    {
        if ((sourceSpan[0] & 0b10000000) == 0b00000000) 
        {
            sourceSpan = sourceSpan.Slice(1);
        } 
        else if ((sourceSpan[0] & 0b11100000) == 0b11000000) 
        {
            sourceSpan = sourceSpan.Slice(2);
        } 
        else if ((sourceSpan[0] & 0b11110000) == 0b11100000) 
        {
            sourceSpan = sourceSpan.Slice(3);
        } 
        else if ((sourceSpan[0] & 0b11111000) == 0b11110000) 
        {
            sourceSpan = sourceSpan.Slice(4);
        }

        size++;
    }

    auto destination = SystemPushArrayZero<wchar_t>(memoryArena, size);
    auto destinationSpan = destination;

    sourceSpan = source;

    while (sourceSpan[0] != '\0')
    {
        if ((sourceSpan[0] & 0b10000000) == 0b00000000) 
        {
            destinationSpan[0] = sourceSpan[0];
            sourceSpan = sourceSpan.Slice(1);
        } 
        else if ((sourceSpan[0] & 0b11100000) == 0b11000000) 
        {
            destinationSpan[0] = ((sourceSpan[0] & 0b00011111) << 6) | (sourceSpan[1] & 0b00111111);
            sourceSpan = sourceSpan.Slice(2);
        } 
        else if ((sourceSpan[0] & 0b11110000) == 0b11100000) 
        {
            destinationSpan[0] = ((sourceSpan[0] & 0b00001111) << 12) | ((sourceSpan[1] & 0b00111111) << 6) | (sourceSpan[2] & 0b00111111);
            sourceSpan = sourceSpan.Slice(3);
        } 
        else if ((sourceSpan[0] & 0b11111000) == 0b11110000) 
        {
            destinationSpan[0] = ((sourceSpan[0] & 0b00000111) << 18) | ((sourceSpan[1] & 0b00111111) << 12) | ((sourceSpan[2] & 0b00111111) << 6) | (sourceSpan[3] & 0b00111111);
            sourceSpan = sourceSpan.Slice(4);
        }

        destinationSpan = destinationSpan.Slice(1);
    }

    return destination;
}

//---------------------------------------------------------------------------------------------------------------
// IO functions
//---------------------------------------------------------------------------------------------------------------

ReadOnlySpan<char> SystemGenerateTempFilename(MemoryArena* memoryArena) 
{
    auto templateName = (ReadOnlySpan<char>)"/tmp/tempfileXXXXXX";
    auto result = SystemPushArrayZero<char>(memoryArena, templateName.Length);
    SystemCopyBuffer(result, templateName);

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
        SystemCopyBuffer<char>(result.Slice(currentLength), buffer.Slice(0, length));

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
