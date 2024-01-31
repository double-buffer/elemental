#include "SystemLogging.h"
#include "SystemMemory.h"
#include "SystemFunctions.h"

static LogMessageHandlerPtr systemLogMessageHandler = nullptr;

void SystemRegisterLogMessageHandler(LogMessageHandlerPtr logMessageHandler)
{
    systemLogMessageHandler = logMessageHandler;
}

void SystemCallLogMessageHandler(ReadOnlySpan<char> functionName, LogMessageType type, LogMessageCategory category, ReadOnlySpan<char> format, ...)
{
    if (systemLogMessageHandler == nullptr)
    {
        return;
    }

    __builtin_va_list arguments;
    __builtin_va_start(arguments, format); 

    auto stackMemoryArena = SystemGetStackMemoryArena();

    auto formattedString = SystemFormatString(stackMemoryArena, format, arguments);
    systemLogMessageHandler(type, category, functionName.Pointer, formattedString.Pointer);

    __builtin_va_end(arguments);
}
