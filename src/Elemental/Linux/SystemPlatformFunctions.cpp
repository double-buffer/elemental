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
        #ifdef ElemAPI
        SystemLogErrorMessage(ElemLogMessageCategory_Application, "Cannot get the current executable path.");
        #endif
        return "";
    }
    
    executablePath[count] = '\0';

    auto result = SystemPushArrayZero<char>(memoryArena, strlen(executablePath));
    SystemCopyBuffer<char>(result, executablePath);

    return result;

}
