#pragma once

#include <functional>
#include "Elemental.h"

#ifdef WIN32
    #define strcpy strcpy_s
#endif

#define ASSERT_LOG(message) ASSERT_TRUE_MSG(strstr(testLogs, message) != NULL, message)

extern bool testForceVulkanApi;
extern bool testHasLogErrors;
extern char testLogs[2048];
extern uint32_t currentTestLogsIndex;
extern ElemGraphicsDevice sharedGraphicsDevice;

static inline void TestLogHandler(ElemLogMessageType messageType, ElemLogMessageCategory category, const char* function, const char* message) 
{
    printf("[");
    printf("\033[36m");

    if (category == ElemLogMessageCategory_Assert)
    {
        printf("Assert");
    }
    else if (category == ElemLogMessageCategory_Memory)
    {
        printf("Memory");
    }
    else if (category == ElemLogMessageCategory_Application)
    {
        printf("Application");
    }
    else if (category == ElemLogMessageCategory_Graphics)
    {
        printf("Graphics");
    }
    else if (category == ElemLogMessageCategory_Inputs)
    {
        printf("Inputs");
    }

    printf("\033[0m]");
    printf("\033[32m %s", function);

    if (messageType == ElemLogMessageType_Error)
    {
        testHasLogErrors = true;
        printf("\033[31m Error:");
    }
    else if (messageType == ElemLogMessageType_Warning)
    {
        printf("\033[33m Warning:");
    }
    else if (messageType == ElemLogMessageType_Debug)
    {
        printf("\033[0m Debug:");
    }
    else
    {
        printf("\033[0m");
    }

    char* logCopyDestination = testLogs + currentTestLogsIndex;
    strcpy(logCopyDestination, message);
    currentTestLogsIndex += strlen(message);

    printf(" %s\033[0m\n", message);
    fflush(stdout);
}

void InitLog()
{
    testHasLogErrors = false;
    currentTestLogsIndex = 0u;
}

ElemGraphicsDevice GetSharedGraphicsDevice()
{
    return sharedGraphicsDevice;
}
