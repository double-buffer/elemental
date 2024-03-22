#include "SystemPlatformFunctions.h"
#include "SystemFunctions.h"

#ifdef ElemAPI
#include "SystemLogging.h"
#else
#define SystemLogErrorMessage(category, format, ...)
#endif 

SystemPlatformAllocationInfos systemPlatformAllocationInfos;

SystemPlatformEnvironment* SystemPlatformGetEnvironment(MemoryArena memoryArena)
{
    auto result = SystemPushStruct<SystemPlatformEnvironment>(memoryArena);
    
    result->PathSeparator = '\\';

    return result;
}

SystemPlatformDateTime* SystemPlatformGetCurrentDateTime(MemoryArena memoryArena)
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

size_t SystemPlatformGetPageSize()
{
    SYSTEM_INFO systemInfo;
    GetSystemInfo(&systemInfo);

    return systemInfo.dwAllocationGranularity;
}

SystemPlatformAllocationInfos SystemPlatformGetAllocationInfos()
{
    return systemPlatformAllocationInfos;
}

void* SystemPlatformReserveMemory(size_t sizeInBytes)
{
    systemPlatformAllocationInfos.ReservedBytes += sizeInBytes;
    return VirtualAlloc2(nullptr, nullptr, sizeInBytes, MEM_RESERVE, PAGE_NOACCESS, nullptr, 0);
}

void SystemPlatformFreeMemory(void* pointer, size_t sizeInBytes)
{
    systemPlatformAllocationInfos.ReservedBytes -= sizeInBytes;
    VirtualFree(pointer, 0, MEM_RELEASE);
}

void SystemPlatformCommitMemory(void* pointer, size_t sizeInBytes)
{
    systemPlatformAllocationInfos.CommittedBytes += sizeInBytes;
    VirtualAlloc2(nullptr, pointer, sizeInBytes, MEM_COMMIT, PAGE_READWRITE, nullptr, 0);
}

void SystemPlatformDecommitMemory(void* pointer, size_t sizeInBytes)
{
    systemPlatformAllocationInfos.CommittedBytes -= sizeInBytes;
    VirtualFree(pointer, sizeInBytes, MEM_DECOMMIT);
}

void SystemPlatformClearMemory(void* pointer, size_t sizeInBytes)
{
    memset(pointer, 0, sizeInBytes);
}

void SystemPlatformCopyMemory(void* destination, const void* source, size_t sizeInBytes)
{
    memcpy(destination, source, sizeInBytes);
}

ReadOnlySpan<char> SystemPlatformGetExecutablePath(MemoryArena memoryArena)
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
        SystemLogErrorMessage(ElemLogMessageCategory_NativeApplication, "Cannot open file %s for reading. (Error code: %d)", path.Pointer, GetLastError());
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
        SystemLogErrorMessage(ElemLogMessageCategory_NativeApplication, "Cannot open file %s for writing. (Error code: %d)", path.Pointer, GetLastError());
        return;
    }

    DWORD bytesWritten;
    if (!WriteFile(fileHandle, data.Pointer, data.Length, &bytesWritten, nullptr) || bytesWritten != data.Length) 
    {
        SystemLogErrorMessage(ElemLogMessageCategory_NativeApplication, "Error writing to file %s. (Error code: %d)", path.Pointer, GetLastError());
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
        SystemLogErrorMessage(ElemLogMessageCategory_NativeApplication, "Cannot open file %s for reading. (Error code: %d)", path.Pointer, GetLastError());
        return;
    }
    
    DWORD bytesRead;
    if (!ReadFile(fileHandle, data.Pointer, data.Length, &bytesRead, nullptr) || bytesRead != data.Length) 
    {
        SystemLogErrorMessage(ElemLogMessageCategory_NativeApplication, "Error reading file %s. (Error code: %d)", path.Pointer, GetLastError());
    }
    
    CloseHandle(fileHandle);
}

void SystemPlatformFileDelete(ReadOnlySpan<char> path)
{
    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto pathWide = SystemConvertUtf8ToWideChar(stackMemoryArena, path);

    if (!DeleteFile(pathWide.Pointer))
    {
        SystemLogErrorMessage(ElemLogMessageCategory_NativeApplication, "Cannot delete file %s. (Error code: %d)", path.Pointer, GetLastError());
    }
}

ReadOnlySpan<char> SystemPlatformExecuteProcess(MemoryArena memoryArena, ReadOnlySpan<char> command)
{
    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto commandWide = SystemConvertUtf8ToWideChar(stackMemoryArena, command);

    SECURITY_ATTRIBUTES pipeAttributes {};
    pipeAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);
    pipeAttributes.bInheritHandle = false;
    pipeAttributes.bInheritHandle = true;

    HANDLE readPipe, writePipe;
    if (!CreatePipe(&readPipe, &writePipe, &pipeAttributes, 0)) 
    {
        SystemLogErrorMessage(ElemLogMessageCategory_NativeApplication, "Cannot open read/write pipe for launching command: %s (Error code: %d)", command.Pointer, GetLastError());
        return ReadOnlySpan<char>();
    }

    STARTUPINFO startupInfo {};
    startupInfo.cb = sizeof(startupInfo);
    startupInfo.hStdOutput = writePipe;
    startupInfo.hStdError = writePipe;
    startupInfo.dwFlags |= STARTF_USESTDHANDLES;

    PROCESS_INFORMATION processInfo {};
    if (!CreateProcess(nullptr, (LPWSTR)commandWide.Pointer, nullptr, nullptr, true, 0, nullptr, nullptr, &startupInfo, &processInfo)) 
    {
        SystemLogErrorMessage(ElemLogMessageCategory_NativeApplication, "Cannot create process for launching command: %s (Error code: %d)", command.Pointer, GetLastError());
        return ReadOnlySpan<char>();
    }

    CloseHandle(writePipe);

    auto result = SystemPushArrayZero<char>(memoryArena, 4096);
    auto buffer = SystemPushArrayZero<char>(stackMemoryArena, 1024);
    auto currentLength = 0;

    DWORD bytesRead;
    while (ReadFile(readPipe, buffer.Pointer, buffer.Length, &bytesRead, nullptr) != 0 && bytesRead != 0) 
    {
        SystemCopyBuffer<char>(result.Slice(currentLength), buffer.Slice(0, bytesRead));
        currentLength += bytesRead;
    }

    CloseHandle(readPipe);
    CloseHandle(processInfo.hProcess);
    CloseHandle(processInfo.hThread);

    return result;
}

void* SystemPlatformLoadLibrary(ReadOnlySpan<char> libraryName)
{
    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto fullName = SystemConcatBuffers<char>(stackMemoryArena, libraryName, ".dll");
    auto fullNameWide = SystemConvertUtf8ToWideChar(stackMemoryArena, fullName);

    return LoadLibrary(fullNameWide.Pointer);
}

void SystemPlatformFreeLibrary(const void* library)
{
    FreeLibrary((HMODULE)library);
}

void* SystemPlatformGetFunctionExport(const void* library, ReadOnlySpan<char> functionName)
{
    return (void*)GetProcAddress((HMODULE)library, functionName.Pointer);
}

void* SystemPlatformCreateThread(void* threadFunction, void* parameters)
{
    auto threadHandle = CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)threadFunction, parameters, 0, nullptr); 

    if (threadHandle == nullptr)
    {
        SystemLogErrorMessage(ElemLogMessageCategory_NativeApplication, "Cannot create thread (Error code: %d)", GetLastError());
        return nullptr;
    }

    return threadHandle;
}

void SystemPlatformWaitThread(void* thread)
{
    if (thread == nullptr)
    {
        SystemLogErrorMessage(ElemLogMessageCategory_NativeApplication, "Invalid thread handle provided.");
        return;
    }

    DWORD waitResult = WaitForSingleObject(thread, INFINITE);

    if (waitResult != WAIT_OBJECT_0)
    {
        SystemLogErrorMessage(ElemLogMessageCategory_NativeApplication, "Failed to wait on thread (Error code: %d)", GetLastError());
    }
}

void SystemPlatformYieldThread()
{
    Sleep(0); 
}

void SystemPlatformFreeThread(void* thread)
{
    CloseHandle(thread);
}
