#include "SystemLogging.h"
#include "SystemMemory.h"
#include "SystemFunctions.h"

static ElemLogHandlerPtr systemLogHandler = nullptr;

void SystemRegisterLogHandler(ElemLogHandlerPtr logHandler)
{
    systemLogHandler = logHandler;
}

void SystemCallLogMessageHandler(ReadOnlySpan<char> functionName, ElemLogMessageType type, ElemLogMessageCategory category, ReadOnlySpan<char> format, ...)
{
    if (systemLogHandler == nullptr)
    {
        return;
    }

    va_list arguments;
    va_start(arguments, format); 

    auto stackMemoryArena = SystemGetStackMemoryArena();

    auto formattedString = SystemFormatString(stackMemoryArena, format, arguments);
    systemLogHandler(type, category, functionName.Pointer, formattedString.Pointer);

    va_end(arguments);
}
