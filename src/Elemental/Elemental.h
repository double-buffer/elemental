#ifndef _ELEMENTAL_H_
#define _ELEMENTAL_H_

#include <stdint.h>
#include <stdbool.h>

#ifndef ElemAPI
#define ElemAPI static
#define UseLoader
#endif

typedef uint64_t ElemHandle;
#define ELEM_HANDLE_NULL 0u

// TODO: Add Functions to get native handlers for app and window so that client code can use interop for other things

//------------------------------------------------------------------------
// ##Module_Application##
//------------------------------------------------------------------------

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

typedef struct
{
    uint8_t* Items;
    uint32_t Length;
} ElemDataSpan;

typedef void (*ElemApplicationHandlerPtr)(void* payload);

typedef struct
{
    const char* ApplicationName;
    ElemApplicationHandlerPtr InitHandler;
    ElemApplicationHandlerPtr FreeHandler;
    void* Payload;
} ElemRunApplicationParameters;

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
 * Configures a custom log handler for the application.
 *
 * @param logHandler The log handler function to be used.
 */
ElemAPI void ElemConfigureLogHandler(ElemLogHandlerPtr logHandler);

// TODO: ElemGetSystemInfo (with System enum, application folder path, etc.)

ElemAPI int32_t ElemRunApplication(const ElemRunApplicationParameters* parameters);

// TODO: ExitApplication();

// TODO: Add the ability to specify window with no decorations for app that needs to render their own ui

/**
 * Creates a window for an application with specified options.
 *
 * @param options Window creation options; uses defaults if NULL.
 * @return A handle to the created window.
 */
ElemAPI ElemWindow ElemCreateWindow(const ElemWindowOptions* options);

/**
 * Frees resources for a specified window. Call when the window is no longer needed.
 *
 * @param window The window instance to free.
 */
ElemAPI void ElemFreeWindow(ElemWindow window);

/**
 * Gets the render size of a window, accounting for DPI scaling.
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
typedef ElemHandle ElemCommandQueue;
typedef ElemHandle ElemCommandList;
typedef ElemHandle ElemSwapChain;
typedef ElemHandle ElemTexture;
typedef ElemHandle ElemShaderLibrary;
typedef ElemHandle ElemPipelineState;

typedef enum
{
    ElemGraphicsApi_DirectX12 = 0,
    ElemGraphicsApi_Vulkan = 1,
    ElemGraphicsApi_Metal = 2
} ElemGraphicsApi;

typedef enum
{
    ElemCommandQueueType_Graphics = 0,
    ElemCommandQueueType_Compute = 1
} ElemCommandQueueType;

typedef enum
{
    ElemSwapChainFormat_Default = 0,
    ElemSwapChainFormat_HighDynamicRange = 1
} ElemSwapChainFormat;

typedef enum
{
    ElemTextureFormat_B8G8R8A8_SRGB
} ElemTextureFormat;

typedef enum
{
    ElemRenderPassLoadAction_Discard = 0,
    ElemRenderPassLoadAction_Load = 1,
    ElemRenderPassLoadAction_Clear = 2
} ElemRenderPassLoadAction;

typedef enum
{
    ElemRenderPassStoreAction_Store = 0,
    ElemRenderPassStoreAction_Discard = 1
} ElemRenderPassStoreAction;

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
} ElemGraphicsDeviceInfoSpan;

typedef struct
{
    uint64_t DeviceId;
} ElemGraphicsDeviceOptions;

typedef struct
{
    const char* DebugName;
} ElemCommandQueueOptions;

typedef struct
{
    const char* DebugName;
} ElemCommandListOptions;

typedef struct
{
    ElemCommandQueue CommandQueue;
    uint64_t FenceValue;
} ElemFence;

typedef struct
{
    ElemCommandList* Items;
    uint32_t Length;
} ElemCommandListSpan;

typedef struct
{
    ElemFence* Items;
    uint32_t Length;
} ElemFenceSpan;

typedef struct
{
    // TODO: Always insert a fence but awaitable on cpu is an option
    bool InsertFence;
    bool FenceAwaitableOnCpu;
    ElemFenceSpan FencesToWait;
} ElemExecuteCommandListOptions;

typedef struct
{
    void* UpdatePayload;
    ElemSwapChainFormat Format;
    uint32_t FrameLatency;
    uint32_t TargetFPS;
} ElemSwapChainOptions;

typedef struct
{
    uint32_t Width;
    uint32_t Height;
    ElemTextureFormat Format;
} ElemSwapChainInfo;

typedef struct
{
    ElemSwapChainInfo SwapChainInfo;
    ElemTexture BackBufferTexture;
    double DeltaTimeInSeconds; 
    double NextPresentTimeStampInSeconds;
} ElemSwapChainUpdateParameters;

typedef struct
{
    ElemTextureFormat* Items;
    uint32_t Length;
} ElemTextureFormatSpan;

// TODO: Implement additional parameters
typedef struct
{
    const char* DebugName;
    ElemShaderLibrary ShaderLibrary;
    const char* MeshShaderFunction;
    const char* PixelShaderFunction;
    ElemTextureFormatSpan TextureFormats;
} ElemGraphicsPipelineStateParameters;

typedef struct
{
    float Red;
    float Green;
    float Blue;
    float Alpha;
} ElemColor;

typedef struct
{
    float X;
    float Y;
    float Width;
    float Height;
    float MinDepth;
    float MaxDepth;
} ElemViewport;

typedef struct
{
    ElemViewport* Items;
    uint32_t Length;
} ElemViewportSpan;

typedef struct
{
    ElemTexture RenderTarget;
    ElemColor ClearColor;
    ElemRenderPassLoadAction LoadAction;
    ElemRenderPassStoreAction StoreAction;
} ElemRenderPassRenderTarget;

typedef struct
{
    ElemRenderPassRenderTarget* Items;
    uint32_t Length;
} ElemRenderPassRenderTargetSpan;

typedef struct
{
    ElemRenderPassRenderTargetSpan RenderTargets;
    ElemViewportSpan Viewports;
} ElemBeginRenderPassParameters;

typedef void (*ElemSwapChainUpdateHandlerPtr)(const ElemSwapChainUpdateParameters* updateParameters, void* payload);

ElemAPI void ElemSetGraphicsOptions(const ElemGraphicsOptions* options);

ElemAPI ElemGraphicsDeviceInfoSpan ElemGetAvailableGraphicsDevices(void);

ElemAPI ElemGraphicsDevice ElemCreateGraphicsDevice(const ElemGraphicsDeviceOptions* options);
ElemAPI void ElemFreeGraphicsDevice(ElemGraphicsDevice graphicsDevice);
ElemAPI ElemGraphicsDeviceInfo ElemGetGraphicsDeviceInfo(ElemGraphicsDevice graphicsDevice);

ElemAPI ElemCommandQueue ElemCreateCommandQueue(ElemGraphicsDevice graphicsDevice, ElemCommandQueueType type, const ElemCommandQueueOptions* options);
ElemAPI void ElemFreeCommandQueue(ElemCommandQueue commandQueue);
ElemAPI void ElemResetCommandAllocation(ElemGraphicsDevice graphicsDevice);
ElemAPI ElemCommandList ElemGetCommandList(ElemCommandQueue commandQueue, const ElemCommandListOptions* options);
ElemAPI void ElemCommitCommandList(ElemCommandList commandList);
ElemAPI ElemFence ElemExecuteCommandList(ElemCommandQueue commandQueue, ElemCommandList commandList, const ElemExecuteCommandListOptions* options);
ElemAPI ElemFence ElemExecuteCommandLists(ElemCommandQueue commandQueue, ElemCommandListSpan commandLists, const ElemExecuteCommandListOptions* options);
ElemAPI void ElemWaitForFenceOnCpu(ElemFence fence);

ElemAPI ElemSwapChain ElemCreateSwapChain(ElemCommandQueue commandQueue, ElemWindow window, ElemSwapChainUpdateHandlerPtr updateHandler, const ElemSwapChainOptions* options);
ElemAPI void ElemFreeSwapChain(ElemSwapChain swapChain);
ElemAPI ElemSwapChainInfo ElemGetSwapChainInfo(ElemSwapChain swapChain);
ElemAPI void ElemSetSwapChainTiming(ElemSwapChain swapChain, uint32_t frameLatency, uint32_t targetFPS);
ElemAPI void ElemPresentSwapChain(ElemSwapChain swapChain);

ElemAPI ElemShaderLibrary ElemCreateShaderLibrary(ElemGraphicsDevice graphicsDevice, ElemDataSpan shaderLibraryData);
ElemAPI void ElemFreeShaderLibrary(ElemShaderLibrary shaderLibrary);
//ElemAPI ElemShaderLibrary ElemCreateShaderLibraryFromShader(ElemShaderType shaderType, ElemDataContainer shaderData, shaderMetadata);
// ElemAPI ElemShaderInfo ElemGetShaderInfo(ElemShaderLibrary shaderLibrary, const char* shaderName);
// ElemAPI ElemShaderInfoList ElemGetShaderLibraryShaders(ElemShaderLibrary shaderLibrary);

// TODO: Provide Async compile methods
// TODO: Provide only one generic pipeline creation method?
ElemAPI ElemPipelineState ElemCompileGraphicsPipelineState(ElemGraphicsDevice graphicsDevice, const ElemGraphicsPipelineStateParameters* parameters);
ElemAPI void ElemFreePipelineState(ElemPipelineState pipelineState);
// TODO: Get Pipeline State Info (for compiled status etc)
//ElemAPI ElemPipelineState ElemCreateComputePipelineState(ElemGraphicsDevice graphicsDevice, const ElemComputePipelineStateParameters* parameters);
// TODO: Enumerate pipeline infos?
ElemAPI void ElemBindPipelineState(ElemCommandList commandList, ElemPipelineState pipelineState);
ElemAPI void ElemPushPipelineStateConstants(ElemCommandList commandList, uint32_t offsetInBytes, ElemDataSpan data); 
// TODO: Cache functions

ElemAPI void ElemBeginRenderPass(ElemCommandList commandList, const ElemBeginRenderPassParameters* parameters);
ElemAPI void ElemEndRenderPass(ElemCommandList commandList);
ElemAPI void ElemSetViewport(ElemCommandList commandList, const ElemViewport* viewport);
ElemAPI void ElemSetViewports(ElemCommandList commandList, ElemViewportSpan viewports);
ElemAPI void ElemDispatchMesh(ElemCommandList commandList, uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ);

// TODO: TDR Logging!
// TODO: Debugging functions (interop with PIX, XCode, etc.)

#ifdef UseLoader
#ifndef ElementalLoader
#include "ElementalLoader.c"
#endif
#endif

#endif  // #ifndef _ELEMENTAL_H_
