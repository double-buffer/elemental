#include "SystemPlatformFunctions.h"
#include "SystemFunctions.h"
#include "SystemLogging.h"

#include <time.h> // TODO: To review
#define MAX_PATH 255

SystemPlatformEnvironment* SystemPlatformGetEnvironment(MemoryArena* memoryArena)
{
    auto result = SystemPushStruct<SystemPlatformEnvironment>(memoryArena);
    
    result->PathSeparator = '/';

    return result;
}

SystemPlatformDateTime* SystemPlatformGetCurrentDateTime(MemoryArena* memoryArena)
{
    time_t currentTime;
    time(&currentTime);

    tm localTime;
    localtime_r(&currentTime, &localTime);

    auto result = SystemPushStruct<SystemPlatformDateTime>(memoryArena);

    result->Year = localTime.tm_year + 1900;
    result->Month = localTime.tm_mon + 1;
    result->Day = localTime.tm_mday;
    result->Hour = localTime.tm_hour;
    result->Minute = localTime.tm_min;
    result->Second = localTime.tm_sec;

    return result;
}

void* SystemPlatformAllocateMemory(size_t sizeInBytes)
{
    auto size = (vm_size_t)sizeInBytes;
    vm_size_t pageSize;
    host_page_size(mach_host_self(), &pageSize);
    size = (size + pageSize - 1) & ~(pageSize - 1);

    vm_address_t address = 0;
    auto result = vm_allocate(mach_task_self(), &address, size, VM_FLAGS_ANYWHERE);

    if (result != KERN_SUCCESS) 
    {
        return nullptr;
    }

    return (void*)address;
}

void SystemPlatformFreeMemory(void* pointer, size_t sizeInBytes)
{
    auto size = (vm_size_t)sizeInBytes;
    vm_size_t pageSize;
    host_page_size(mach_host_self(), &pageSize);
    size = (size + pageSize - 1) & ~(pageSize - 1);

    auto address = (vm_address_t)pointer;
    vm_deallocate(mach_task_self(), address, size);
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
    auto path = (ReadOnlySpan<char>)NS::Bundle::mainBundle()->executablePath()->utf8String();
    auto result = SystemPushArrayZero<char>(memoryArena, path.Length);

    SystemCopyBuffer(result, path);

    return result;
}

bool SystemPlatformFileExists(ReadOnlySpan<char> path)
{
    struct stat buffer;
    return (stat(path.Pointer, &buffer) == 0);
}

size_t SystemPlatformFileGetSizeInBytes(ReadOnlySpan<char> path)
{
    struct stat buffer;

    if (stat(path.Pointer, &buffer) != 0) 
    {
        SystemLogErrorMessage(LogMessageCategory_NativeApplication, "Cannot open file %s for reading.", path.Pointer);
        return 0;
    }

    return buffer.st_size;
}

void SystemPlatformFileWriteBytes(ReadOnlySpan<char> path, ReadOnlySpan<uint8_t> data)
{
    auto fileHandle = open(path.Pointer, O_WRONLY | O_CREAT, 0644);

    if (fileHandle < 0) 
    {
        SystemLogErrorMessage(LogMessageCategory_NativeApplication, "Cannot open file %s for writing.", path.Pointer);
        return;
    }
    
    if (write(fileHandle, data.Pointer, data.Length) < 0) 
    {
        SystemLogErrorMessage(LogMessageCategory_NativeApplication, "Error writing to file %s.", path.Pointer);
    }

    close(fileHandle);
}

void SystemPlatformFileReadBytes(ReadOnlySpan<char> path, Span<uint8_t> data)
{
    auto fileHandle = open(path.Pointer, O_WRONLY | O_CREAT, 0644);

    if (fileHandle < 0) 
    {
        SystemLogErrorMessage(LogMessageCategory_NativeApplication, "Cannot open file %s for reading.", path.Pointer);
        return;
    }

    auto bytesRead = read(fileHandle, data.Pointer, data.Length);

    if (bytesRead < 0) 
    {
        SystemLogErrorMessage(LogMessageCategory_NativeApplication, "Error reading file %s.", path.Pointer);
    }

    close(fileHandle);
}

void SystemPlatformFileDelete(ReadOnlySpan<char> path)
{
    if (unlink(path.Pointer) != 0) 
    {
        SystemLogErrorMessage(LogMessageCategory_NativeApplication, "Cannot delete file %s.", path.Pointer);
    }
}

ReadOnlySpan<char> SystemPlatformExecuteProcess(MemoryArena* memoryArena, ReadOnlySpan<char> command)
{
    // TODO: To review
    auto stackMemoryArena = SystemGetStackMemoryArena();

    int pipefd[2];

    if (pipe(pipefd) == -1) 
    {
        SystemLogErrorMessage(LogMessageCategory_NativeApplication, "Cannot open pipe for launching command: %s", command.Pointer);
        return ReadOnlySpan<char>();
    }

    pid_t pid = fork();
    if (pid == -1) 
    {
        SystemLogErrorMessage(LogMessageCategory_NativeApplication, "Cannot fork process for launching command: %s", command.Pointer);
        close(pipefd[0]);
        close(pipefd[1]);
        return ReadOnlySpan<char>();
    } 
    else if (pid == 0) 
    {
        // Child process
        close(pipefd[0]); // Close unused read end
        dup2(pipefd[1], STDOUT_FILENO); // Redirect stdout to write end of the pipe
        dup2(pipefd[1], STDERR_FILENO); // Redirect stderr to write end of the pipe
        close(pipefd[1]);

        // Convert command from ReadOnlySpan<char> to a null-terminated string if necessary
        // Execute the command
        execl("/bin/sh", "sh", "-c", command.Pointer, nullptr);
        exit(EXIT_FAILURE); // execl only returns on error
    }

    // Parent process
    close(pipefd[1]); // Close unused write end

    auto result = SystemPushArrayZero<char>(memoryArena, 4096);
    auto buffer = SystemPushArrayZero<char>(stackMemoryArena, 1024);
    auto currentLength = 0;
    ssize_t bytesRead;

    while ((bytesRead = read(pipefd[0], buffer.Pointer, buffer.Length)) > 0) 
    {
        SystemCopyBuffer<char>(result.Slice(currentLength), buffer.Slice(0, bytesRead));
        currentLength += bytesRead;
    }

    close(pipefd[0]);
    waitpid(pid, nullptr, 0); // Wait for child process to finish

    return result;
}

void* SystemPlatformLoadLibrary(ReadOnlySpan<char> libraryName)
{
    auto stackMemoryArena = SystemGetStackMemoryArena();
    auto fullName = SystemConcatBuffers<char>(stackMemoryArena, libraryName, "dylib");

    return dlopen(fullName.Pointer, RTLD_NOW);
}

void SystemPlatformFreeLibrary(const void* library)
{
    dlclose((void*)library);
}

void* SystemPlatformGetFunctionExport(const void* library, ReadOnlySpan<char> functionName)
{
    return dlsym((void*)library, functionName.Pointer);
}

void* SystemPlatformCreateThread(void* threadFunction, void* parameters)
{
    pthread_t thread;
    auto result = pthread_create(&thread, nullptr, (void* (*)(void*))threadFunction, parameters);

    if (result != 0) 
    {
        SystemLogErrorMessage(LogMessageCategory_NativeApplication, "Cannot create thread (Error code: %d)", result);
        return nullptr;
    }

    pthread_detach(thread);
    return *(void**)(&thread);
}

void SystemPlatformFreeThread(void* thread)
{
}
