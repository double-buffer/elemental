#ifndef _ELEMENTAL_H_
#define _ELEMENTAL_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#ifndef ElemAPI
#define ElemAPI static
#define UseLoader
#endif

#define ElemHandle uint64_t
#define ElemApplication ElemHandle

typedef enum
{
    ElemLogMessageType_Debug = 0,
    ElemLogMessageType_Warning = 1,
    ElemLogMessageType_Error = 2
} ElemLogMessageType;

typedef enum
{
    ElemLogMessageCategory_Memory = 0,
    ElemLogMessageCategory_NativeApplication = 1,
    ElemLogMessageCategory_Graphics = 2,
    ElemLogMessageCategory_Inputs = 3
} ElemLogMessageCategory;

typedef void (*ElemLogHandlerPtr)(ElemLogMessageType messageType, ElemLogMessageCategory category, const char* function, const char* message);

typedef struct
{
    uint32_t Status;
} ElemApplicationStatus;

typedef bool (*ElemRunHandlerPtr)(ElemApplicationStatus status);

// ##Module_ElementalApplication##
/*
*    This function help
*/
ElemAPI void ElemConfigureLogHandler(ElemLogHandlerPtr logHandler);
ElemAPI ElemApplication ElemCreateApplication(const char* applicationName);
ElemAPI void ElemRunApplication(ElemApplication application, ElemRunHandlerPtr runHandler);
ElemAPI void ElemFreeApplication(ElemApplication application);

#ifdef UseLoader
#ifndef ElementalLoader
#include "ElementalLoader.c"
#endif
#endif

void ElemConsoleLogHandler(ElemLogMessageType messageType, ElemLogMessageCategory category, const char* function, const char* message) 
{
    printf("[");
    printf("\033[36m");

    // TODO: Provide a mapping function
    if (category == ElemLogMessageCategory_Memory)
    {
        printf("Memory");
    }
    else if (category == ElemLogMessageCategory_NativeApplication)
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
        printf("\033[31m");
    }
    else if (messageType == ElemLogMessageType_Warning)
    {
        printf("\033[33m");
    }
    else
    {
        printf("\033[0m");
    }

    printf(" %s\n\033[0m", message);
}

#endif  // #ifndef _ELEMENTAL_H_
