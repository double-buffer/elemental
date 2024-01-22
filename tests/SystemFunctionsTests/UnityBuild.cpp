#include "SystemPlatformFunctions.cpp"
#include "SystemLogging.cpp"
#include "SystemMemory.cpp"
#include "SystemFunctions.cpp"
#include "SystemDictionary.cpp"
#include "SystemInputs.cpp"

#include "utest.h"

#include "MemoryTests.cpp"
#include "MathTests.cpp"
#include "StringTests.cpp"
#include "IOTests.cpp"
#include "LibraryProcessTests.cpp"
#include "DictionaryTests.cpp"

#ifdef _DEBUG
void LogMessageHandler(LogMessageType messageType, LogMessageCategory category, const char* function, const char* message)
{
    if (messageType == LogMessageType_Error)
    {
        printf("\033[31mERROR: ");
    }
    else if (messageType == LogMessageType_Warning)
    {
        printf("\033[33mWARNING: ");
    }
    else if (messageType == LogMessageType_Debug)
    {
        printf("\033[36mDEBUG: ");
    }

    printf("%s: %s\n\033[0m", function, message);
}
#endif

UTEST_STATE();

int main(int argc, const char *const argv[]) 
{
    #ifdef _DEBUG
    SystemRegisterLogMessageHandler(LogMessageHandler);
    #endif
    return utest_main(argc, argv);
}
