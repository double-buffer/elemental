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
    printf("%s: %s\n", function, message);
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
