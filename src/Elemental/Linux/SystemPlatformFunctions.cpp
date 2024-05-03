#include "SystemPlatformFunctions.h"

#ifdef ElemAPI
#include "SystemLogging.h"
#else
#define SystemLogErrorMessage(category, format, ...)
#endif 

ReadOnlySpan<char> SystemPlatformGetExecutablePath(MemoryArena memoryArena)
{
    char executablePath[PATH_MAX];
    auto count = readlink("/proc/self/exe", executablePath, PATH_MAX);

    if (count == -1) 
    {
        SystemLogErrorMessage(ElemLogMessageCategory_Application, "Cannot get the current executable path.");
        return "";
    }
    
    executablePath[count] = '\0';
    auto directory = dirname(executablePath);

    auto result = SystemPushArrayZero<char>(memoryArena, strlen(directory));
    SystemCopyBuffer<char>(result, directory);

    return result;

}
