#ifndef _ELEMENTAL_H_
#define _ELEMENTAL_H_

#include <stdint.h>
#include <stdbool.h>

#ifndef ElemAPI
#define ElemAPI static
#define UseLoader
#endif

typedef uint64_t ElemHandle;


//------------------------------------------------------------------------
// ##Module_Application##
//------------------------------------------------------------------------
/**
 * Handle that represents an elemental application. 
 */
typedef ElemHandle ElemApplication;
typedef ElemHandle ElemWindow;

/**
 * Enumerates the types of log messages.
 */
typedef enum
{
    // Debugging message.
    ElemLogMessageType_Debug = 0,
    // Warning message.
    ElemLogMessageType_Warning = 1,
    // Error message.
    ElemLogMessageType_Error = 2
} ElemLogMessageType;

/**
 * Enumerates the categories of log messages.
 */
typedef enum
{
    // Memory related messages.
    ElemLogMessageCategory_Assert = 0,
    // Memory related messages.
    ElemLogMessageCategory_Memory = 1,
    // Native application messages.
    ElemLogMessageCategory_NativeApplication = 2,
    // Graphics system messages.
    ElemLogMessageCategory_Graphics = 3,
    // Input system messages.
    ElemLogMessageCategory_Inputs = 4
} ElemLogMessageCategory;

/**
 * Represents the status of an application.
 */
typedef enum
{
    // Active status.
    ElemApplicationStatus_Active = 0,
    // Closing status.
    ElemApplicationStatus_Closing = 1
} ElemApplicationStatus;

typedef enum
{
    ElemWindowState_Normal = 0,
    ElemWindowState_Minimized = 1,
    ElemWindowState_Maximized = 2,
    ElemWindowState_FullScreen = 3
} ElemWindowState;

typedef struct
{
    const char* Title;
    int32_t Width;
    int32_t Height;
    ElemWindowState WindowState;
} ElemWindowOptions;

typedef struct
{
    int32_t Width;
    int32_t Height;
    float UIScale;
} ElemWindowSize;

/**
 * Defines a function pointer type for log handling.
 * @param messageType The type of the log message.
 * @param category The category of the log message.
 * @param function The function where the log was triggered.
 * @param message The log message.
 */
typedef void (*ElemLogHandlerPtr)(ElemLogMessageType messageType, ElemLogMessageCategory category, const char* function, const char* message);

/**
 * Defines a function pointer type for application run handling.
 * @param status The current application status.
 * @return Returns true to continue running, false to exit.
 */
typedef bool (*ElemRunHandlerPtr)(ElemApplicationStatus status);

/**
 * Configures a custom log handler for the application.
 * @param logHandler The log handler function to be used.
 */
ElemAPI void ElemConfigureLogHandler(ElemLogHandlerPtr logHandler);

/**
 * Creates a new application instance.
 * @param applicationName The name of the application.
 * @return Returns an application handle.
 */
ElemAPI ElemApplication ElemCreateApplication(const char* applicationName);

/**
 * Frees resources associated with the given application.
 * @param application The application to free.
 */
ElemAPI void ElemFreeApplication(ElemApplication application);

/**
 * Runs the specified application with the provided run handler.
 * @param application The application to run.
 * @param runHandler The function to call on each run iteration.
 */
ElemAPI void ElemRunApplication(ElemApplication application, ElemRunHandlerPtr runHandler);


ElemAPI ElemWindow ElemCreateWindow(ElemApplication application, const ElemWindowOptions* options);

ElemAPI void ElemFreeWindow(ElemWindow window);

ElemAPI ElemWindowSize ElemGetWindowRenderSize(ElemWindow window);

ElemAPI void ElemSetWindowTitle(ElemWindow window, const char* title);

ElemAPI void ElemSetWindowState(ElemWindow window, ElemWindowState windowState);


//------------------------------------------------------------------------
// ##Module_Graphics##
//------------------------------------------------------------------------
/**
 * Handle that represents a graphics device. 
 */
typedef ElemHandle ElemGraphicsDevice;

/**
 * Temporary Function
 */
ElemAPI ElemGraphicsDevice ElemCreateGraphicsDevice(void);

/**
 * Temporary Function
 */
ElemAPI void ElemFreeGraphicsDevice(ElemGraphicsDevice graphicsDevice);


#ifdef UseLoader
#ifndef ElementalLoader
#include "ElementalLoader.c"
#endif
#endif

#endif  // #ifndef _ELEMENTAL_H_
