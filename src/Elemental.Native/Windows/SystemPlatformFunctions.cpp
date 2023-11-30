#include "SystemPlatformFunctions.h"
#include "SystemFunctions.h"
#include "SystemLogging.h"

#include <stdio.h> // TEMP

SystemPlatformEnvironment* SystemPlatformGetEnvironment(MemoryArena* memoryArena)
{
    auto result = SystemPushStruct<SystemPlatformEnvironment>(memoryArena);
    
    result->PathSeparator = '\\';

    return result;
}

SystemPlatformDateTime* SystemPlatformGetCurrentDateTime(MemoryArena* memoryArena)
{
    SYSTEMTIME systemTime;
    GetLocalTime(&systemTime);

    auto result = SystemPushStruct<SystemPlatformDateTime>(memoryArena);

    result->Year = systemTime.wYear;
    result->Month = systemTime.wMonth;
    result->Day = systemTime.wDay;
    result->Hour = systemTime.wHour;
    result->Minute = systemTime.wMinute;
    result->Second = systemTime.wSecond;

    return result;
}

void* SystemPlatformAllocateMemory(size_t sizeInBytes)
{
    return malloc(sizeInBytes);
}

void SystemPlatformFreeMemory(void* pointer)
{
    free(pointer);
}

void SystemPlatformClearMemory(void* pointer, size_t sizeInBytes)
{
    memset(pointer, 0, sizeInBytes);
}

void SystemPlatformCopyMemory(void* destination, const void* source, size_t sizeInBytes)
{
    memcpy(destination, source, sizeInBytes);
}

ReadOnlySpan<char> SystemPlatformGetExecutablePath(MemoryArena* memoryArena)
{
    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto path = SystemPushArray<wchar_t>(stackMemoryArena, MAX_PATH);

    GetModuleFileName(nullptr, path.Pointer, MAX_PATH);

    return SystemConvertWideCharToUtf8(memoryArena, path);
}

bool SystemPlatformFileExists(ReadOnlySpan<char> path)
{
    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto pathWide = SystemConvertUtf8ToWideChar(stackMemoryArena, path);
    auto attributes = GetFileAttributes(pathWide.Pointer);

    return (attributes != INVALID_FILE_ATTRIBUTES && !(attributes & FILE_ATTRIBUTE_DIRECTORY));
}

size_t SystemPlatformFileGetSizeInBytes(ReadOnlySpan<char> path)
{
    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto pathWide = SystemConvertUtf8ToWideChar(stackMemoryArena, path);

    auto fileHandle = CreateFile(pathWide.Pointer, GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

    if (fileHandle == INVALID_HANDLE_VALUE) 
    {
        SystemLogErrorMessage(LogMessageCategory_NativeApplication, "Cannot open file %s for reading.", path.Pointer);
        return 0;
    }
    
    auto fileSize = GetFileSize(fileHandle, nullptr);
    
    CloseHandle(fileHandle);
    return fileSize;
}

void SystemPlatformFileWriteBytes(ReadOnlySpan<char> path, ReadOnlySpan<uint8_t> data)
{
    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto pathWide = SystemConvertUtf8ToWideChar(stackMemoryArena, path);
    
    auto fileHandle = CreateFile(pathWide.Pointer, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

    if (fileHandle == INVALID_HANDLE_VALUE) 
    {
        SystemLogErrorMessage(LogMessageCategory_NativeApplication, "Cannot open file %s for writing.", path.Pointer);
        return;
    }

    DWORD bytesWritten;
    if (!WriteFile(fileHandle, data.Pointer, data.Length, &bytesWritten, nullptr) || bytesWritten != data.Length) 
    {
        SystemLogErrorMessage(LogMessageCategory_NativeApplication, "Error writing to file %s.", path.Pointer);
    }

    CloseHandle(fileHandle);
}

void SystemPlatformFileReadBytes(ReadOnlySpan<char> path, Span<uint8_t> data)
{
    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto pathWide = SystemConvertUtf8ToWideChar(stackMemoryArena, path);

    auto fileHandle = CreateFile(pathWide.Pointer, GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

    if (fileHandle == INVALID_HANDLE_VALUE) 
    {
        SystemLogErrorMessage(LogMessageCategory_NativeApplication, "Cannot open file %s for reading.", path.Pointer);
        return;
    }
    
    DWORD bytesRead;
    if (!ReadFile(fileHandle, data.Pointer, data.Length, &bytesRead, nullptr) || bytesRead != data.Length) 
    {
        SystemLogErrorMessage(LogMessageCategory_NativeApplication, "Error reading file %s.", path.Pointer);
    }
    
    CloseHandle(fileHandle);
}

void SystemPlatformFileDelete(ReadOnlySpan<char> path)
{
    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto pathWide = SystemConvertUtf8ToWideChar(stackMemoryArena, path);

    if (!DeleteFile(pathWide.Pointer))
    {
        SystemLogErrorMessage(LogMessageCategory_NativeApplication, "Cannot delete file %s.", path.Pointer);
    }
}
