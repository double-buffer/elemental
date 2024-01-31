#pragma once

#include "ElementalCommon.h"
#include "SystemSpan.h"

/**
 * Registers a log message handler function.
 *
 * This function allows the registration of a custom log message handler, which will be called
 * whenever a log message is generated in the system.
 *
 * @param logMessageHandler A pointer to the custom log message handler function.
 */
void SystemRegisterLogMessageHandler(LogMessageHandlerPtr logMessageHandler);

/**
 * Calls the registered log message handler with the specified log message details.
 *
 * This function is used internally to pass log messages to the registered log message handler.
 *
 * @param functionName The name of the function where the log message was generated.
 * @param type The type of the log message (e.g., Debug, Warning, Error).
 * @param category The category of the log message.
 * @param format The format string for the log message.
 * @param ... Additional parameters for formatting the log message.
 */
void SystemCallLogMessageHandler(ReadOnlySpan<char> functionName, LogMessageType type, LogMessageCategory category, ReadOnlySpan<char> format, ...);

/**
 * Macro for logging a custom log message.
 *
 * This macro simplifies the logging process by automatically providing the current function name.
 *
 * @param type The type of the log message (e.g., Debug, Warning, Error).
 * @param category The category of the log message.
 * @param format The format string for the log message.
 * @param ... Additional parameters for formatting the log message.
 */
#define SystemLogMessage(type, category, format, ...) SystemCallLogMessageHandler(__FUNCTION__, type, category, format, ##__VA_ARGS__)

/**
 * Macro for logging a debug-level log message.
 *
 * This macro is a convenience wrapper for SystemLogMessage, setting the log type to Debug.
 *
 * @param category The category of the log message.
 * @param format The format string for the log message.
 * @param ... Additional parameters for formatting the log message.
 */
#define SystemLogDebugMessage(category, format, ...) SystemCallLogMessageHandler(__FUNCTION__, LogMessageType_Debug, category, format, ##__VA_ARGS__)

/**
 * Macro for logging a warning-level log message.
 *
 * This macro is a convenience wrapper for SystemLogMessage, setting the log type to Warning.
 *
 * @param category The category of the log message.
 * @param format The format string for the log message.
 * @param ... Additional parameters for formatting the log message.
 */
#define SystemLogWarningMessage(category, format, ...) SystemCallLogMessageHandler(__FUNCTION__, LogMessageType_Warning, category, format, ##__VA_ARGS__)

/**
 * Macro for logging an error-level log message.
 *
 * This macro is a convenience wrapper for SystemLogMessage, setting the log type to Error.
 *
 * @param category The category of the log message.
 * @param format The format string for the log message.
 * @param ... Additional parameters for formatting the log message.
 */
#define SystemLogErrorMessage(category, format, ...) SystemCallLogMessageHandler(__FUNCTION__, LogMessageType_Error, category, format, ##__VA_ARGS__)