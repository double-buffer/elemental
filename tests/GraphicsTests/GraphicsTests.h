#pragma once

#include "Elemental.h"

#ifdef WIN32
    #define strcpy strcpy_s
#endif

extern bool testForceVulkanApi;
extern bool testHasLogErrors;

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
    else if (category == ElemLogMessageCategory_NativeApplication)
    {
        printf("NativeApplication");
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

    printf(" %s\033[0m\n", message);
    fflush(stdout);
}

void InitLog()
{
    testHasLogErrors = false;

    ElemGraphicsOptions options = { .PreferVulkan = testForceVulkanApi };

    #ifdef _DEBUG
    ElemConfigureLogHandler(TestLogHandler);
    options.EnableDebugLayer = true;
    #else
    ElemConfigureLogHandler(ElemConsoleErrorLogHandler);
    #endif

    ElemSetGraphicsOptions(&options);
}
