//--------------------------------------------------------------------------------
// Elemental Library
// Version: 1.0.0-dev4
//
// MIT License
//
// Copyright (c) 2023-2024 Double Buffer SRL
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//--------------------------------------------------------------------------------

#ifndef _ELEMENTAL_H_
#define _ELEMENTAL_H_

#include <stdint.h>
#include <stdbool.h>

#ifndef ElemAPI
#define ElemAPI static
#define UseLoader
#endif

#define ELEM_VERSION_LABEL "1.0.0-dev4"

typedef uint64_t ElemHandle;
#define ELEM_HANDLE_NULL 0u

//--------------------------------------------------------------------------------
// ##Module_Application##
//--------------------------------------------------------------------------------

/**
 * Represents a handle to an elemental window.
 */
typedef ElemHandle ElemWindow;

/**
 * Defines the types of log messages that can be generated.
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
 * Categorizes log messages by their related system components.
 */
typedef enum
{
    // Assertions and checks.
    ElemLogMessageCategory_Assert = 0,
    // Memory allocation and management.
    ElemLogMessageCategory_Memory = 1,
    // General application behavior.
    ElemLogMessageCategory_Application = 2,
    // Graphics system-related messages.
    ElemLogMessageCategory_Graphics = 3,
    // Input system-related messages.
    ElemLogMessageCategory_Inputs = 4
} ElemLogMessageCategory;

/**
 * Lists the platforms supported by the library.
 */
typedef enum
{
    // Microsoft Windows platform.
    ElemPlatform_Windows = 0,
    // Apple macOS platform.
    ElemPlatform_MacOS = 1,
    // Apple iOS platform.
    ElemPlatform_iOS = 2,
    // Linux platform.
    ElemPlatform_Linux = 3
} ElemPlatform;

/**
 * Enumerates the possible states of a window.
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
 * Represents a span of data, typically used for handling arrays or blocks of memory.
 */
typedef struct
{
    // Pointer to the data items.
    uint8_t* Items;
    // Number of items in the span.
    uint32_t Length;
} ElemDataSpan;

/**
 * Defines a function pointer type for handling application events.
 */
typedef void (*ElemApplicationHandlerPtr)(void* payload);

/**
 * Holds parameters for running an application, including initialization and cleanup routines.
 */
typedef struct
{
    // Name of the application.
    const char* ApplicationName;
    // Function called at application startup.
    ElemApplicationHandlerPtr InitHandler;
    // Function called at application termination.
    ElemApplicationHandlerPtr FreeHandler;
    // Custom user data passed to handler functions.
    void* Payload;
} ElemRunApplicationParameters;

/**
 * Contains information about the system, useful for tailoring application behavior.
 */
typedef struct
{
    // Operating system platform.
    ElemPlatform Platform;
    // Installation path of the application.
    const char* ApplicationPath;
    // Whether the application supports multiple windows.
    bool SupportMultiWindows;
} ElemSystemInfo;

/**
 * Defines options for creating a new window.
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
    // True if the cursor should be hidden.
    bool IsCursorHidden;
} ElemWindowOptions;

/**
 * Contains detailed information about the size and scaling factors of a window's render area.
 */
typedef struct
{
    // Width of the window's render area in pixels.
    uint32_t Width;
    // Height of the window's render area in pixels.
    uint32_t Height;
    // UI scale factor, typically used for DPI adjustments.
    float UIScale;
    // Current state of the window.
    ElemWindowState WindowState;
} ElemWindowSize;

// TODO: Comments
typedef struct
{
    uint32_t X;
    uint32_t Y;
} ElemWindowCursorPosition;

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
 * Configures a custom log handler for processing log messages generated by the application.
 *
 * @param logHandler The function to call when a log message is generated.
 */
ElemAPI void ElemConfigureLogHandler(ElemLogHandlerPtr logHandler);

/**
 * Retrieves system-related information, such as platform and application path.
 *
 * @return A structure containing system information.
 */
ElemAPI ElemSystemInfo ElemGetSystemInfo(void);

/**
 * Starts the execution of an application with specified parameters.
 *
 * @param parameters Configuration and handlers for the application lifecycle.
 * @return Status code indicating success or error.
 */
ElemAPI int32_t ElemRunApplication(const ElemRunApplicationParameters* parameters);

/**
 * Exits the application, performing necessary cleanup.
 *
 * @param exitCode Exit code of the application.
 */
ElemAPI void ElemExitApplication(int32_t exitCode);

/**
 * Creates a window with the specified options or default settings if none are provided.
 *
 * @param options Configuration options for the window; NULL for defaults.
 * @return A handle to the newly created window.
 */
ElemAPI ElemWindow ElemCreateWindow(const ElemWindowOptions* options);

/**
 * Releases resources associated with a window.
 *
 * @param window Handle to the window to be freed.
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

// TODO: Comments
// TODO: Make sure the coordinates are consistent accross all platforms
ElemAPI void ElemShowWindowCursor(ElemWindow window);
ElemAPI void ElemHideWindowCursor(ElemWindow window);
ElemAPI ElemWindowCursorPosition ElemGetWindowCursorPosition(ElemWindow window);

//--------------------------------------------------------------------------------
// ##Module_Graphics##
//--------------------------------------------------------------------------------

 /**
 * Handle that represents a graphics device.
 */
typedef ElemHandle ElemGraphicsDevice;

/**
 * Handle that represents a command queue.
 */
typedef ElemHandle ElemCommandQueue;

/**
 * Handle that represents a command list.
 */
typedef ElemHandle ElemCommandList;

/**
 * Handle that represents a swap chain.
 */
typedef ElemHandle ElemSwapChain;

/**
 * Handle that represents a texture.
 */
typedef ElemHandle ElemTexture;

/**
 * Handle that represents a shader library.
 */
typedef ElemHandle ElemShaderLibrary;

/**
 * Handle that represents a pipeline state.
 */
typedef ElemHandle ElemPipelineState;

/**
 * Enumerates supported graphics APIs.
 */
typedef enum
{
    // Represents DirectX 12 API.
    ElemGraphicsApi_DirectX12 = 0,
    // Represents Vulkan API.
    ElemGraphicsApi_Vulkan = 1,
    // Represents Metal API.
    ElemGraphicsApi_Metal = 2
} ElemGraphicsApi;

/**
 * Enumerates types of command queues.
 */
typedef enum
{
    // Command queue for graphics operations.
    ElemCommandQueueType_Graphics = 0,
    // Command queue for compute operations.
    ElemCommandQueueType_Compute = 1
} ElemCommandQueueType;

/**
 * Enumerates swap chain formats.
 */
typedef enum
{
    // Default format.
    ElemSwapChainFormat_Default = 0,
    // High dynamic range format.
    ElemSwapChainFormat_HighDynamicRange = 1
} ElemSwapChainFormat;

/**
 * Enumerates texture formats.
 */
typedef enum
{
    // Standard 8-bit BGRA format using sRGB color space.
    ElemTextureFormat_B8G8R8A8_SRGB
} ElemTextureFormat;

/**
 * Enumerates render pass load actions.
 */
typedef enum
{
    // Discards the previous contents.
    ElemRenderPassLoadAction_Discard = 0,
    // Loads the existing contents.
    ElemRenderPassLoadAction_Load = 1,
    // Clears to a predefined value.
    ElemRenderPassLoadAction_Clear = 2
} ElemRenderPassLoadAction;

/**
 * Enumerates render pass store actions.
 */
typedef enum
{
    // Preserves the contents after rendering.
    ElemRenderPassStoreAction_Store = 0,
    // Discards the contents after rendering.
    ElemRenderPassStoreAction_Discard = 1
} ElemRenderPassStoreAction;

/**
 * Configuration options for graphics initialization.
 */
typedef struct
{
    // Enable debugging features if set to true.
    bool EnableDebugLayer;
    // Prefer using Vulkan API if set to true.
    bool PreferVulkan;
} ElemGraphicsOptions;

/**
 * Information about a graphics device.
 */
typedef struct
{
    // TODO: Add ElemGraphicsDevice Handle

    // Name of the graphics device.
    const char* DeviceName;
    // API used by the graphics device.
    ElemGraphicsApi GraphicsApi;
    // Unique identifier for the device.
    uint64_t DeviceId;
    // Available memory on the device.
    uint64_t AvailableMemory;
} ElemGraphicsDeviceInfo;

/**
 * Represents a collection of graphics device information.
 */
typedef struct
{
    // Pointer to an array of ElemGraphicsDeviceInfo.
    ElemGraphicsDeviceInfo* Items;
    // Number of items in the array.
    uint32_t Length;
} ElemGraphicsDeviceInfoSpan;

/**
 * Options for creating a graphics device.
 */
typedef struct
{
    // Identifier for a specific device to be initialized.
    uint64_t DeviceId;
} ElemGraphicsDeviceOptions;

/**
 * Options for creating a command queue.
 */
typedef struct
{
    // Optional debug name for the command queue.
    const char* DebugName;
} ElemCommandQueueOptions;

/**
 * Options for creating a command list.
 */
typedef struct
{
    // Optional debug name for the command list.
    const char* DebugName;
} ElemCommandListOptions;

/**
 * Represents a fence for command list synchronization.
 */
typedef struct
{
    // Associated command queue for the fence.
    ElemCommandQueue CommandQueue;
    // The fence value to be reached.
    uint64_t FenceValue;
} ElemFence;

/**
 * Represents a collection of command lists.
 */
typedef struct
{
    // Pointer to an array of ElemCommandList.
    ElemCommandList* Items;
    // Number of items in the array.
    uint32_t Length;
} ElemCommandListSpan;

/**
 * Represents a collection of fences.
 */
typedef struct
{
    // Pointer to an array of ElemFence.
    ElemFence* Items;
    // Number of items in the array.
    uint32_t Length;
} ElemFenceSpan;

/**
 * Options for executing a command list.
 */
typedef struct
{
    // TODO: Do we need that? This was foreseen for metal because SharedEvent may be slower
    // But it would be great to avoid this flag.
    // If set to true, CPU can wait on the fence.
    bool FenceAwaitableOnCpu;
    // Fences that the execution should wait on before starting.
    ElemFenceSpan FencesToWait;
} ElemExecuteCommandListOptions;

/**
 * Options for configuring a swap chain.
 */
typedef struct
{
    // Custom payload for swap chain updates.
    void* UpdatePayload;
    // Format of the swap chain.
    ElemSwapChainFormat Format;
    // Desired maximum number of frames in flight.
    uint32_t FrameLatency;
    // Target frames per second.
    uint32_t TargetFPS;
} ElemSwapChainOptions;

/**
 * Information about the swap chain setup.
 */
typedef struct
{
    // Width of the swap chain in pixels.
    uint32_t Width;
    // Height of the swap chain in pixels.
    uint32_t Height;
    // Aspect ratio of the swap chain.
    float AspectRatio;
    // Format of the textures used in the swap chain.
    ElemTextureFormat Format;
} ElemSwapChainInfo;

/**
 * Parameters for updating a swap chain during rendering.
 */
typedef struct
{
    // Information about the swap chain's configuration.
    ElemSwapChainInfo SwapChainInfo;
    // Back buffer texture for the swap chain.
    ElemTexture BackBufferTexture;
    // Time since the last frame was presented, in seconds.
    double DeltaTimeInSeconds; 
    // Timestamp for when the next frame is expected to be presented, in seconds.
    double NextPresentTimestampInSeconds;
} ElemSwapChainUpdateParameters;

/**
 * Represents a collection of texture formats.
 */
typedef struct
{
    // Pointer to an array of ElemTextureFormat.
    ElemTextureFormat* Items;
    // Number of items in the array.
    uint32_t Length;
} ElemTextureFormatSpan;

/**
 * Parameters for creating a graphics pipeline state.
 */
typedef struct
{
    // Optional debug name for the pipeline state.
    const char* DebugName;
    // Shader library containing the shaders.
    ElemShaderLibrary ShaderLibrary;
    // Function name of the mesh shader in the shader library.
    const char* MeshShaderFunction;
    // Function name of the pixel shader in the shader library.
    const char* PixelShaderFunction;
    // Supported texture formats for the pipeline state.
    ElemTextureFormatSpan TextureFormats;
} ElemGraphicsPipelineStateParameters;

/**
 * Represents RGBA color.
 */
typedef struct
{
    // Red component.
    float Red;
    // Green component.
    float Green;
    // Blue component.
    float Blue;
    // Alpha component.
    float Alpha; 
} ElemColor;

/**
 * Defines a viewport for rendering.
 */
typedef struct
{
    // X coordinate of the viewport's top left corner.
    float X;
    // Y coordinate of the viewport's top left corner.
    float Y;
    // Width of the viewport.       
    float Width;
    // Height of the viewport.   
    float Height;
    // Minimum depth of the viewport.  
    float MinDepth;
    // Maximum depth of the viewport.
    float MaxDepth; 
} ElemViewport;

/**
 * Represents a collection of viewports.
 */
typedef struct
{
    // Pointer to an array of ElemViewport.
    ElemViewport* Items;
    // Number of items in the array.
    uint32_t Length;
} ElemViewportSpan;

/**
 * Configuration for a render pass target.
 */
typedef struct
{
    // Render target texture.
    ElemTexture RenderTarget;
    // Color to clear the render target with if the load action is clear.
    ElemColor ClearColor;
    // Action to take when loading data into the render target at the beginning of a render pass.
    ElemRenderPassLoadAction LoadAction;
    // Action to take when storing data from the render target at the end of a render pass.
    ElemRenderPassStoreAction StoreAction;
} ElemRenderPassRenderTarget;

/**
 * Represents a collection of render pass targets.
 */
typedef struct
{
    // Pointer to an array of ElemRenderPassRenderTarget.
    ElemRenderPassRenderTarget* Items;
    // Number of items in the array.
    uint32_t Length;
} ElemRenderPassRenderTargetSpan;

/**
 * Parameters for beginning a render pass.
 */
typedef struct
{
    // Render targets to be used in the render pass.
    ElemRenderPassRenderTargetSpan RenderTargets;
    // Viewports to be used in the render pass.
    ElemViewportSpan Viewports;
} ElemBeginRenderPassParameters;

/**
 * Function pointer type for handling updates to the swap chain.
 */
typedef void (*ElemSwapChainUpdateHandlerPtr)(const ElemSwapChainUpdateParameters* updateParameters, void* payload);

/**
 * Sets graphics options like enabling a debug layer and choosing preferred graphics API.
 * @param options Configuration options for initializing the graphics system.
 */
ElemAPI void ElemSetGraphicsOptions(const ElemGraphicsOptions* options);

/**
 * Retrieves a list of available graphics devices on the system.
 * @return A span of graphics device information, encapsulating details about each available device.
 */
ElemAPI ElemGraphicsDeviceInfoSpan ElemGetAvailableGraphicsDevices(void);

/**
 * Creates a graphics device based on specified options.
 * @param options Configuration options for the graphics device to be created.
 * @return A handle to the newly created graphics device.
 */
ElemAPI ElemGraphicsDevice ElemCreateGraphicsDevice(const ElemGraphicsDeviceOptions* options);

/**
 * Releases resources associated with a graphics device.
 * @param graphicsDevice The graphics device to free.
 */
ElemAPI void ElemFreeGraphicsDevice(ElemGraphicsDevice graphicsDevice);

/**
 * Retrieves detailed information about a specific graphics device.
 * @param graphicsDevice The graphics device to query.
 * @return A structure containing detailed information about the device.
 */
ElemAPI ElemGraphicsDeviceInfo ElemGetGraphicsDeviceInfo(ElemGraphicsDevice graphicsDevice);

/**
 * Creates a command queue of a specified type on a graphics device.
 * @param graphicsDevice The device on which to create the command queue.
 * @param type The type of the command queue, such as graphics or compute.
 * @param options Additional options for creating the command queue.
 * @return A handle to the newly created command queue.
 */
ElemAPI ElemCommandQueue ElemCreateCommandQueue(ElemGraphicsDevice graphicsDevice, ElemCommandQueueType type, const ElemCommandQueueOptions* options);

/**
 * Releases resources associated with a command queue.
 * @param commandQueue The command queue to free.
 */
ElemAPI void ElemFreeCommandQueue(ElemCommandQueue commandQueue);

/**
 * Resets all command allocations on a graphics device, typically used to reset the state between frames.
 * @param graphicsDevice The device whose allocations are to be reset.
 */
ElemAPI void ElemResetCommandAllocation(ElemGraphicsDevice graphicsDevice);

/**
 * Retrieves a command list from a command queue, configured according to provided options.
 * @param commandQueue The command queue from which to retrieve the command list.
 * @param options Additional options for creating the command list.
 * @return A handle to the retrieved command list.
 */
ElemAPI ElemCommandList ElemGetCommandList(ElemCommandQueue commandQueue, const ElemCommandListOptions* options);

/**
 * Commits a command list for execution, finalizing its commands and preparing them to be executed on the GPU.
 * @param commandList The command list to commit.
 */
ElemAPI void ElemCommitCommandList(ElemCommandList commandList);

/**
 * Executes a single command list on a command queue, optionally waiting for specified fences before execution.
 * @param commandQueue The command queue on which to execute the command list.
 * @param commandList The command list to execute.
 * @param options Additional options controlling execution, such as fence dependencies.
 * @return A fence indicating the completion state of the command list's execution.
 */
ElemAPI ElemFence ElemExecuteCommandList(ElemCommandQueue commandQueue, ElemCommandList commandList, const ElemExecuteCommandListOptions* options);

/**
 * Executes multiple command lists on a command queue, optionally waiting for specified fences before execution.
 * @param commandQueue The command queue on which to execute the command lists.
 * @param commandLists A span of command lists to be executed.
 * @param options Additional options controlling execution, such as fence dependencies.
 * @return A fence indicating the completion state of the command lists' execution.
 */
ElemAPI ElemFence ElemExecuteCommandLists(ElemCommandQueue commandQueue, ElemCommandListSpan commandLists, const ElemExecuteCommandListOptions* options);

/**
 * Waits for a fence to reach its signaled state on the CPU, effectively synchronizing CPU and GPU operations.
 * @param fence The fence to wait on.
 */
ElemAPI void ElemWaitForFenceOnCpu(ElemFence fence);

/**
 * Creates a swap chain for a window, allowing rendered frames to be presented to the screen.
 * @param commandQueue The command queue associated with rendering commands for the swap chain.
 * @param window The window for which the swap chain is to be created.
 * @param updateHandler A callback function that is called when the swap chain is updated.
 * @param options Additional options for configuring the swap chain.
 * @return A handle to the newly created swap chain.
 */
ElemAPI ElemSwapChain ElemCreateSwapChain(ElemCommandQueue commandQueue, ElemWindow window, ElemSwapChainUpdateHandlerPtr updateHandler, const ElemSwapChainOptions* options);

/**
 * Releases resources associated with a swap chain.
 * @param swapChain The swap chain to free.
 */
ElemAPI void ElemFreeSwapChain(ElemSwapChain swapChain);

/**
 * Retrieves current information about a swap chain, such as its dimensions and format.
 * @param swapChain The swap chain to query.
 * @return A structure containing detailed information about the swap chain.
 */
ElemAPI ElemSwapChainInfo ElemGetSwapChainInfo(ElemSwapChain swapChain);

/**
 * Sets timing parameters for a swap chain, controlling its frame latency and target frame rate.
 * @param swapChain The swap chain to configure.
 * @param frameLatency The maximum number of frames that can be queued for display.
 * @param targetFPS The target frames per second to aim for during rendering.
 */
ElemAPI void ElemSetSwapChainTiming(ElemSwapChain swapChain, uint32_t frameLatency, uint32_t targetFPS);

/**
 * Presents the next frame in the swap chain, updating the display with new content.
 * @param swapChain The swap chain from which to present the frame.
 */
ElemAPI void ElemPresentSwapChain(ElemSwapChain swapChain);

/**
 * Creates a shader library from provided binary data, allowing shaders to be loaded and used by graphics pipeline states.
 * @param graphicsDevice The device on which to create the shader library.
 * @param shaderLibraryData The binary data containing the shaders.
 * @return A handle to the newly created shader library.
 */
ElemAPI ElemShaderLibrary ElemCreateShaderLibrary(ElemGraphicsDevice graphicsDevice, const ElemDataSpan shaderLibraryData);

/**
 * Releases resources associated with a shader library.
 * @param shaderLibrary The shader library to free.
 */
ElemAPI void ElemFreeShaderLibrary(ElemShaderLibrary shaderLibrary);

/**
 * Compiles a graphics pipeline state using specified shaders and configuration.
 * @param graphicsDevice The device on which to compile the pipeline state.
 * @param parameters Parameters defining the pipeline state configuration.
 * @return A handle to the newly compiled pipeline state.
 */
ElemAPI ElemPipelineState ElemCompileGraphicsPipelineState(ElemGraphicsDevice graphicsDevice, const ElemGraphicsPipelineStateParameters* parameters);

/**
 * Releases resources associated with a pipeline state.
 * @param pipelineState The pipeline state to free.
 */
ElemAPI void ElemFreePipelineState(ElemPipelineState pipelineState);

/**
 * Binds a compiled pipeline state to a command list, preparing it for rendering operations.
 * @param commandList The command list to which the pipeline state is to be bound.
 * @param pipelineState The pipeline state to bind.
 */
ElemAPI void ElemBindPipelineState(ElemCommandList commandList, ElemPipelineState pipelineState);

/**
 * Pushes constants to a bound pipeline state, allowing for quick updates to shader constants without re-binding or modifying buffers.
 * @param commandList The command list through which the constants are pushed.
 * @param offsetInBytes The offset within the constant buffer at which to begin updating constants.
 * @param data The data to be pushed as constants.
 */
ElemAPI void ElemPushPipelineStateConstants(ElemCommandList commandList, uint32_t offsetInBytes, const ElemDataSpan data);

/**
 * Begins a render pass, setting up the rendering targets and viewports for drawing operations.
 * @param commandList The command list on which the render pass is to be started.
 * @param parameters The parameters defining the render pass configuration.
 */
ElemAPI void ElemBeginRenderPass(ElemCommandList commandList, const ElemBeginRenderPassParameters* parameters);

/**
 * Ends the current render pass on a command list, finalizing drawing operations and preparing for presentation or further rendering.
 * @param commandList The command list on which the render pass is to be ended.
 */
ElemAPI void ElemEndRenderPass(ElemCommandList commandList);

/**
 * Sets a single viewport for rendering on a command list.
 * @param commandList The command list on which the viewport is to be set.
 * @param viewport The viewport configuration to be applied.
 */
ElemAPI void ElemSetViewport(ElemCommandList commandList, const ElemViewport* viewport);

/**
 * Sets multiple viewports for rendering on a command list.
 * @param commandList The command list on which the viewports are to be set.
 * @param viewports A span of viewports to be applied.
 */
ElemAPI void ElemSetViewports(ElemCommandList commandList, ElemViewportSpan viewports);

/**
 * Dispatches a mesh shader operation on a command list, specifying the number of thread groups in each dimension.
 * @param commandList The command list on which the mesh operation is to be dispatched.
 * @param threadGroupCountX The number of thread groups in the X dimension.
 * @param threadGroupCountY The number of thread groups in the Y dimension.
 * @param threadGroupCountZ The number of thread groups in the Z dimension.
 */
ElemAPI void ElemDispatchMesh(ElemCommandList commandList, uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ);

//--------------------------------------------------------------------------------
// ##Module_Inputs##
//--------------------------------------------------------------------------------

typedef ElemHandle ElemInputDevice;

typedef enum
{
    ElemInputDeviceType_Unknown,
    ElemInputDeviceType_Keyboard,
    ElemInputDeviceType_Mouse,
    ElemInputDeviceType_Gamepad,
} ElemInputDeviceType;

// TODO: Assign values to enum
// TODO: Review the enums to see if it still ok at the end of the implementation
typedef enum
{
    ElemInputType_Digital,
    ElemInputType_Analog,
    ElemInputType_Delta,
} ElemInputType;

typedef enum
{
    ElemInputId_Unknown,
    ElemInputId_KeyTilde,
    ElemInputId_Key1,
    ElemInputId_Key2,
    ElemInputId_Key3,
    ElemInputId_Key4,
    ElemInputId_Key5,
    ElemInputId_Key6,
    ElemInputId_Key7,
    ElemInputId_Key8,
    ElemInputId_Key9,
    ElemInputId_Key0,
    ElemInputId_KeyDash,
    ElemInputId_KeyEquals,
    ElemInputId_KeyBackspace,
    ElemInputId_KeyTab,
    ElemInputId_KeyQ,
    ElemInputId_KeyW,
    ElemInputId_KeyE,
    ElemInputId_KeyR,
    ElemInputId_KeyT,
    ElemInputId_KeyY,
    ElemInputId_KeyU,
    ElemInputId_KeyI,
    ElemInputId_KeyO,
    ElemInputId_KeyP,
    ElemInputId_KeyLeftBrace,
    ElemInputId_KeyRightBrace,
    ElemInputId_KeyBackSlash,
    ElemInputId_KeyCapsLock,
    ElemInputId_KeyA,
    ElemInputId_KeyS,
    ElemInputId_KeyD,
    ElemInputId_KeyF,
    ElemInputId_KeyG,
    ElemInputId_KeyH,
    ElemInputId_KeyJ,
    ElemInputId_KeyK,
    ElemInputId_KeyL,
    ElemInputId_KeySemiColon,
    ElemInputId_KeyApostrophe,
    ElemInputId_KeyEnter,
    ElemInputId_KeyLeftShift,
    ElemInputId_KeyZ,
    ElemInputId_KeyX,
    ElemInputId_KeyC,
    ElemInputId_KeyV,
    ElemInputId_KeyB,
    ElemInputId_KeyN,
    ElemInputId_KeyM,
    ElemInputId_KeyComma,
    ElemInputId_KeyPeriod,
    ElemInputId_KeySlash,
    ElemInputId_KeyRightShift,
    ElemInputId_KeyLeftControl,
    ElemInputId_KeyLeftAlt,
    ElemInputId_KeySpacebar,
    ElemInputId_KeyRightAlt,
    ElemInputId_KeyRightControl,
    ElemInputId_KeyInsert,
    ElemInputId_KeyDelete,
    ElemInputId_KeyLeftArrow,
    ElemInputId_KeyHome,
    ElemInputId_KeyEnd,
    ElemInputId_KeyUpArrow,
    ElemInputId_KeyDownArrow,
    ElemInputId_KeyPageUp,
    ElemInputId_KeyPageDown,
    ElemInputId_KeyRightArrow,
    ElemInputId_KeyNumpadLock,
    ElemInputId_KeyNumpad7,
    ElemInputId_KeyNumpad4,
    ElemInputId_KeyNumpad1,
    ElemInputId_KeyNumpadDivide,
    ElemInputId_KeyNumpad8,
    ElemInputId_KeyNumpad5,
    ElemInputId_KeyNumpad2,
    ElemInputId_KeyNumpad0,
    ElemInputId_KeyNumpadMultiply,
    ElemInputId_KeyNumpad9,
    ElemInputId_KeyNumpad6,
    ElemInputId_KeyNumpad3,
    ElemInputId_KeyNumpadSeparator,
    ElemInputId_KeyNumpadMinus,
    ElemInputId_KeyNumpadAdd,
    ElemInputId_KeyNumpadEnter,
    ElemInputId_KeyEscape,
    ElemInputId_KeyF1,
    ElemInputId_KeyF2,
    ElemInputId_KeyF3,
    ElemInputId_KeyF4,
    ElemInputId_KeyF5,
    ElemInputId_KeyF6,
    ElemInputId_KeyF7,
    ElemInputId_KeyF8,
    ElemInputId_KeyF9,
    ElemInputId_KeyF10,
    ElemInputId_KeyF11,
    ElemInputId_KeyF12,
    ElemInputId_KeyPrintScreen,
    ElemInputId_KeyScrollLock,
    ElemInputId_KeyPause,
    ElemInputId_KeyLeftSystem,
    ElemInputId_KeyRightSystem,
    ElemInputId_KeyApp,
    ElemInputId_MouseLeftButton,
    ElemInputId_MouseRightButton,
    ElemInputId_MouseMiddleButton,
    ElemInputId_MouseExtraButton1,
    ElemInputId_MouseExtraButton2,
    ElemInputId_MouseAxisXNegative,
    ElemInputId_MouseAxisXPositive,
    ElemInputId_MouseAxisYNegative,
    ElemInputId_MouseAxisYPositive,
    ElemInputId_MouseWheelNegative,
    ElemInputId_MouseWheelPositive,
    ElemInputId_MouseHorizontalWheelNegative,
    ElemInputId_MouseHorizontalWheelPositive,
    ElemInputID_GamepadButtonA,
    ElemInputID_GamepadButtonB,
    ElemInputID_GamepadButtonX,
    ElemInputID_GamepadButtonY,
    ElemInputID_GamepadButtonMenu,
    ElemInputID_GamepadButtonOptions,
    ElemInputID_GamepadButtonHome,
    ElemInputID_GamepadLeftShoulder,
    ElemInputID_GamepadRightShoulder,
    ElemInputID_GamepadLeftTrigger,
    ElemInputID_GamepadRightTrigger,
    ElemInputId_GamepadLeftStickXNegative,
    ElemInputId_GamepadLeftStickXPositive,
    ElemInputId_GamepadLeftStickYNegative,
    ElemInputId_GamepadLeftStickYPositive,
    ElemInputId_GamepadLeftStickButton,
    ElemInputId_GamepadRightStickXNegative,
    ElemInputId_GamepadRightStickXPositive,
    ElemInputId_GamepadRightStickYNegative,
    ElemInputId_GamepadRightStickYPositive,
    ElemInputId_GamepadRightStickButton,
    ElemInputId_GamepadDpadUp,
    ElemInputId_GamepadDpadRight,
    ElemInputId_GamepadDpadDown,
    ElemInputId_GamepadDpadLeft,
} ElemInputId;

typedef struct
{
    ElemInputDevice Handle;
    ElemInputDeviceType DeviceType;
} ElemInputDeviceInfo;

typedef struct
{
    ElemInputDeviceInfo* Items;
    uint32_t Length;
} ElemInputDeviceInfoSpan;

typedef struct
{
    ElemWindow Window;
    ElemInputDevice InputDevice;
    ElemInputId InputId;
    ElemInputType InputType;
    float Value;
    double ElapsedSeconds;
} ElemInputEvent;

typedef struct
{
    ElemInputEvent* Items;
    uint32_t Length;
} ElemInputEventSpan;

typedef struct
{
    ElemInputEventSpan Events;
} ElemInputStream;

ElemAPI ElemInputDeviceInfo ElemGetInputDeviceInfo(ElemInputDevice inputDevice);
ElemAPI ElemInputStream ElemGetInputStream(void);

#ifdef UseLoader
#ifndef ElementalLoader
#include "ElementalLoader.c"
#endif
#endif

#endif  // #ifndef _ELEMENTAL_H_
