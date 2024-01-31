#ifndef _ELEMENTAL_H_
#define _ELEMENTAL_H_

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

#endif  // #ifndef _ELEMENTAL_H_
