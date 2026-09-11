#include "SystemLogging.h"
#include "SystemMemory.h"
#include "SystemFunctions.h"

static ElemLogHandlerPtr systemLogHandler = nullptr;
thread_local bool systemLogMessageInProgress = false;

void SystemRegisterLogHandler(ElemLogHandlerPtr logHandler)
{
    systemLogHandler = logHandler;
}

void SystemCallLogMessageHandler(ReadOnlySpan<char> functionName, ElemLogMessageType type, ElemLogMessageCategory category, ReadOnlySpan<char> format, ...)
{
    if (systemLogHandler == nullptr || systemLogMessageInProgress)
    {
        return;
    }

    systemLogMessageInProgress = true;

    va_list arguments;
    va_start(arguments, format);

    auto stackMemoryArena = SystemGetStackMemoryArena();

    if (stackMemoryArena.Scope.Arena.Storage != nullptr)
    {
        auto formattedString = SystemFormatString(stackMemoryArena, format, arguments);
        systemLogHandler(type, category, functionName.Pointer, formattedString.Pointer);
    }

    va_end(arguments);
    systemLogMessageInProgress = false;
}
