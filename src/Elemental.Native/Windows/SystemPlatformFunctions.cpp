#include "SystemPlatformFunctions.h"
#include "SystemFunctions.h"

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
