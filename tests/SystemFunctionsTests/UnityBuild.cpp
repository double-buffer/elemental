#include "utest.h"

#include "MemoryTests.cpp"
#include "MathTests.cpp"
#include "StringTests.cpp"
#include "IOTests.cpp"
#include "LibraryProcessTests.cpp"
#include "DictionaryTests.cpp"
#include "DataPoolTests.cpp"

#include "SystemPlatformFunctions.cpp"
#include "SystemLogging.cpp"
#include "SystemMemory.cpp"
#include "SystemFunctions.cpp"
#include "SystemDictionary.cpp"
#include "SystemDataPool.cpp"
#include "SystemInputs.cpp"

#ifdef _DEBUG
void LogMessageHandler(ElemLogMessageType messageType, ElemLogMessageCategory category, const char* function, const char* message)
{
    if (messageType == ElemLogMessageType_Error)
    {
        printf("\033[31mERROR: ");
    }
    else if (messageType == ElemLogMessageType_Warning)
    {
        printf("\033[33mWARNING: ");
    }
    else if (messageType == ElemLogMessageType_Debug)
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
    SystemRegisterLogHandler(LogMessageHandler);
    #endif
    return utest_main(argc, argv);
}
