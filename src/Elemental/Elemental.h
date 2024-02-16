#ifndef _ELEMENTAL_H_
#define _ELEMENTAL_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#ifndef ElemAPI
#define ElemAPI static
#define UseLoader
#endif

typedef uint64_t ElemHandle;
typedef ElemHandle ElemApplication;

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

// ##Module_Application##
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

#endif  // #ifndef _ELEMENTAL_H_
