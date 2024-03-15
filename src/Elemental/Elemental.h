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

/**
 * Handle that represents an elemental window. 
 */
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

/**
 * Enumerates the possible window states.
 */
typedef enum
{
    // Window is in normal state.
    ElemWindowState_Normal = 0,
    // Window is minimized.
    ElemWindowState_Minimized = 1,
    // Window is maximized.
    ElemWindowState_Maximized = 2,
    // Window is in full screen mode.
    ElemWindowState_FullScreen = 3
} ElemWindowState;

/**
 * Specifies options for window creation.
 */
typedef struct
{
    // Title of the window.
    const char* Title;
    // Width of the window in pixels.
    uint32_t Width;
    // Height of the window in pixels.
    uint32_t Height; 
    // Initial state of the window.
    ElemWindowState WindowState;
} ElemWindowOptions;

/**
 * Contains information about the size and scale of a window.
 */
typedef struct
{
    // Width of the window's render area in pixels.
    uint32_t Width;
    // Height of the window's render area in pixels.
    uint32_t Height;
    // Scale factor for the UI, useful for DPI adjustments.
    float UIScale;
    // Current state of the window.
    ElemWindowState WindowState;
} ElemWindowSize;

/**
 * Defines a function pointer type for log handling.
 *
 * @param messageType The type of the log message.
 * @param category The category of the log message.
 * @param function The function where the log was triggered.
 * @param message The log message.
 */
typedef void (*ElemLogHandlerPtr)(ElemLogMessageType messageType, ElemLogMessageCategory category, const char* function, const char* message);

/**
 * Defines a function pointer type for application run handling.
 *
 * @param status The current application status.
 * @return Returns true to continue running, false to exit.
 */
typedef bool (*ElemRunHandlerPtr)(ElemApplicationStatus status);

/**
 * Configures a custom log handler for the application.
 *
 * @param logHandler The log handler function to be used.
 */
ElemAPI void ElemConfigureLogHandler(ElemLogHandlerPtr logHandler);

/**
 * Creates a new application instance.
 *
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
 *
 * @param application The application to run.
 * @param runHandler The function to call on each run iteration.
 */
ElemAPI void ElemRunApplication(ElemApplication application, ElemRunHandlerPtr runHandler);

/**
 * Creates a window for an application with specified options.
 *
 * @param application The associated application instance.
 * @param options Window creation options; uses defaults if NULL.
 * @return A handle to the created window.
 */
ElemAPI ElemWindow ElemCreateWindow(ElemApplication application, const ElemWindowOptions* options);

/**
 * Frees resources for a specified window. Call when the window is no longer needed.
 *
 * @param window The window instance to free.
 */
ElemAPI void ElemFreeWindow(ElemWindow window);

/**
 * Gets the render size of a window, accounting for DPI scaling.
 *
 * @param window The window instance.
 * @return Render size of the window.
 */
ElemAPI ElemWindowSize ElemGetWindowRenderSize(ElemWindow window);

/**
 * Sets a window's title.
 *
 * @param window The window instance.
 * @param title New title for the window.
 */
ElemAPI void ElemSetWindowTitle(ElemWindow window, const char* title);

/**
 * Changes the state of a window (e.g., minimize, maximize).
 *
 * @param window The window instance.
 * @param windowState New state for the window.
 */
ElemAPI void ElemSetWindowState(ElemWindow window, ElemWindowState windowState);


//------------------------------------------------------------------------
// ##Module_Graphics##
//------------------------------------------------------------------------
/**
 * Handle that represents a graphics device. 
 */
typedef ElemHandle ElemGraphicsDevice;

typedef enum
{
    ElemGraphicsApi_Direct3D12,
    ElemGraphicsApi_Vulkan
} ElemGraphicsApi;

typedef struct
{
    bool EnableDebugLayer;
    bool PreferVulkan;
} ElemGraphicsOptions;

typedef struct
{
    const char* DeviceName;
    ElemGraphicsApi GraphicsApi;
    uint64_t DeviceId;
    uint64_t AvailableMemory;
} ElemGraphicsDeviceInfo;

typedef struct
{
    ElemGraphicsDeviceInfo* Items;
    uint32_t Length;
} ElemGraphicsDeviceInfoList;

typedef struct
{
    uint64_t DeviceId;
} ElemGraphicsDeviceOptions;

ElemAPI void ElemSetGraphicsOptions(const ElemGraphicsOptions* options);

ElemAPI ElemGraphicsDeviceInfoList ElemGetAvailableGraphicsDevices(void);

ElemAPI ElemGraphicsDevice ElemCreateGraphicsDevice(const ElemGraphicsDeviceOptions* options);
ElemAPI void ElemFreeGraphicsDevice(ElemGraphicsDevice graphicsDevice);
ElemAPI ElemGraphicsDeviceInfo ElemGetGraphicsDeviceInfo(ElemGraphicsDevice graphicsDevice);


#ifdef UseLoader
#ifndef ElementalLoader
#include "ElementalLoader.c"
#endif
#endif

#endif  // #ifndef _ELEMENTAL_H_
