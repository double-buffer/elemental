//--------------------------------------------------------------------------------
// Elemental Library
// Version: 1.0.0-dev5
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

#define ELEM_VERSION_LABEL "1.0.0-dev5"

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

// TODO: Do a ElemGetLastError() function?

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

typedef ElemHandle ElemIOCommandQueue;

/**
 * Handle that represents a swap chain.
 */
typedef ElemHandle ElemSwapChain;

/**
 * Handle that represents a graphics heap.
 */
typedef ElemHandle ElemGraphicsHeap;

/**
 * Handle that represents a graphics resource.
 */
typedef ElemHandle ElemGraphicsResource;

/**
 * Handle that represents a graphics resource descriptor.
 */
typedef int32_t ElemGraphicsResourceDescriptor;

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

typedef enum
{
    ElemIOCommandType_File = 0,
    ElemIOCommandType_Memory = 1
} ElemIOCommandType;

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

typedef enum
{
    ElemGraphicsHeapType_Gpu = 0,
    ElemGraphicsHeapType_GpuUpload = 1,
    ElemGraphicsHeapType_Readback = 2
} ElemGraphicsHeapType;

/**
 * Enumerates graphics formats.
 */
typedef enum
{
    ElemGraphicsFormat_Raw,
    ElemGraphicsFormat_B8G8R8A8_SRGB,
    ElemGraphicsFormat_B8G8R8A8_UNORM,
    ElemGraphicsFormat_R16G16B16A16_FLOAT,
    ElemGraphicsFormat_R32G32B32A32_FLOAT,
    ElemGraphicsFormat_D32_FLOAT 
} ElemGraphicsFormat;

typedef enum
{
    ElemGraphicsResourceType_Buffer,
    ElemGraphicsResourceType_Texture2D
} ElemGraphicsResourceType;

typedef enum
{
    ElemGraphicsResourceUsage_Read = 0x00,
    ElemGraphicsResourceUsage_Write = 0x01,
    ElemGraphicsResourceUsage_RenderTarget = 0x02,
    ElemGraphicsResourceUsage_DepthStencil = 0x04
} ElemGraphicsResourceUsage;

typedef enum
{
    ElemGraphicsResourceDescriptorUsage_Read = 0x00,
    ElemGraphicsResourceDescriptorUsage_Write = 0x01
} ElemGraphicsResourceDescriptorUsage;

typedef enum
{
    ElemGraphicsFillMode_Solid = 0,
    ElemGraphicsFillMode_Wireframe = 1
} ElemGraphicsFillMode;

typedef enum
{
    ElemGraphicsCullMode_BackFace = 0,
    ElemGraphicsCullMode_FrontFace = 1,
    ElemGraphicsCullMode_None = 2
} ElemGraphicsCullMode;

typedef enum
{
    ElemGraphicsBlendOperation_Add = 0,
    ElemGraphicsBlendOperation_Subtract = 1,
    ElemGraphicsBlendOperation_ReverseSubtract = 2,
    ElemGraphicsBlendOperation_Min = 3,
    ElemGraphicsBlendOperation_Max = 4,
} ElemGraphicsBlendOperation;

typedef enum
{
    ElemGraphicsBlendFactor_Zero = 0,
    ElemGraphicsBlendFactor_One = 1,
    ElemGraphicsBlendFactor_SourceColor = 2,
    ElemGraphicsBlendFactor_InverseSourceColor = 3,
    ElemGraphicsBlendFactor_SourceAlpha = 4,
    ElemGraphicsBlendFactor_InverseSourceAlpha = 5,
    ElemGraphicsBlendFactor_DestinationColor = 6,
    ElemGraphicsBlendFactor_InverseDestinationColor = 7,
    ElemGraphicsBlendFactor_DestinationAlpha = 8,
    ElemGraphicsBlendFactor_InverseDestinationAlpha = 9,
    ElemGraphicsBlendFactor_SourceAlphaSaturated = 10,
} ElemGraphicsBlendFactor;

typedef enum
{
    ElemGraphicsCompareFunction_Never = 0,
    ElemGraphicsCompareFunction_Less = 1,
    ElemGraphicsCompareFunction_Equal = 2,
    ElemGraphicsCompareFunction_LessEqual = 3,
    ElemGraphicsCompareFunction_Greater = 4,
    ElemGraphicsCompareFunction_NotEqual = 5,
    ElemGraphicsCompareFunction_GreaterEqual = 6,
    ElemGraphicsCompareFunction_Always = 7
} ElemGraphicsCompareFunction;

typedef enum
{
    ElemGraphicsResourceBarrierSyncType_None,
    ElemGraphicsResourceBarrierSyncType_Compute,
    ElemGraphicsResourceBarrierSyncType_RenderTarget
} ElemGraphicsResourceBarrierSyncType;

typedef enum
{
    ElemGraphicsResourceBarrierAccessType_NoAccess,
    ElemGraphicsResourceBarrierAccessType_Read,
    ElemGraphicsResourceBarrierAccessType_Write,
    ElemGraphicsResourceBarrierAccessType_RenderTarget,
    ElemGraphicsResourceBarrierAccessType_DepthStencilWrite
} ElemGraphicsResourceBarrierAccessType;

typedef enum 
{
    ElemGraphicsResourceBarrierLayoutType_Undefined,
    ElemGraphicsResourceBarrierLayoutType_Read,
    ElemGraphicsResourceBarrierLayoutType_Write,
    ElemGraphicsResourceBarrierLayoutType_RenderTarget,
    ElemGraphicsResourceBarrierLayoutType_DepthStencilWrite,
    ElemGraphicsResourceBarrierLayoutType_Present
} ElemGraphicsResourceBarrierLayoutType;

/**
 * Enumerates render pass load actions.
 */
typedef enum
{
    // Clears to a predefined value.
    ElemRenderPassLoadAction_Clear = 0,
    // Discards the previous contents.
    ElemRenderPassLoadAction_Discard = 1,
    // Loads the existing contents.
    ElemRenderPassLoadAction_Load = 2,
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
    // Enable GPU validation if debug layer is enabled.
    bool EnableGpuValidation;
    // Enable debug logging of barriers.
    bool EnableDebugBarrierInfo;
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

typedef struct
{
    // Optional debug name for the command queue.
    const char* DebugName;
} ElemIOCommandQueueOptions;

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
    // Fences that the execution should wait on before starting.
    ElemFenceSpan FencesToWait;
} ElemExecuteCommandListOptions;

typedef struct
{
    ElemIOCommandType IOCommandType; 
} ElemIOCommandParameters;

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
    ElemWindow Window;
    // Width of the swap chain in pixels.
    uint32_t Width;
    // Height of the swap chain in pixels.
    uint32_t Height;
    // Aspect ratio of the swap chain.
    float AspectRatio;
    float UIScale;
    // Format of the textures used in the swap chain.
    ElemGraphicsFormat Format;
} ElemSwapChainInfo;

/**
 * Parameters for updating a swap chain during rendering.
 */
typedef struct
{
    // Information about the swap chain's configuration.
    ElemSwapChainInfo SwapChainInfo;
    // Back buffer render target for the swap chain.
    ElemGraphicsResource BackBufferRenderTarget;
    // Time since the last frame was presented, in seconds.
    double DeltaTimeInSeconds; 
    // Timestamp for when the next frame is expected to be presented, in seconds.
    double NextPresentTimestampInSeconds;
    // True if the size of the swapchain has changed from the previous update.
    bool SizeChanged;
} ElemSwapChainUpdateParameters;

/**
 * Options for creating a graphics heap.
 */
typedef struct
{
    // Type of the graphics heap. Default to GPU.
    ElemGraphicsHeapType HeapType;
    // Optional debug name for the graphics heap.
    const char* DebugName;
} ElemGraphicsHeapOptions;

typedef struct
{
    const char* DebugName;
} ElemGraphicsResourceInfoOptions;

// TODO: Mip Levels
typedef struct
{
    ElemGraphicsResourceType Type;
    uint32_t Width;
    uint32_t Height;
    uint32_t MipLevels;
    ElemGraphicsFormat Format;
    uint32_t Alignment;
    uint32_t SizeInBytes;
    ElemGraphicsResourceUsage Usage;
    const char* DebugName;
} ElemGraphicsResourceInfo;

typedef struct
{
    // Fences that the execution should wait on before starting.
    ElemFenceSpan FencesToWait;
} ElemFreeGraphicsResourceOptions;

// TODO: Here, we could add options to support StructuredBuffer (we need a different stride for that)
typedef struct
{
    uint32_t TextureMipIndex;
} ElemGraphicsResourceDescriptorOptions;

typedef struct
{
    ElemGraphicsResource Resource;
    ElemGraphicsResourceDescriptorUsage Usage;
    uint32_t TextureMipIndex;
} ElemGraphicsResourceDescriptorInfo;

typedef struct
{
    // Fences that the execution should wait on before starting.
    ElemFenceSpan FencesToWait;
} ElemFreeGraphicsResourceDescriptorOptions;

typedef struct
{
    ElemGraphicsFormat Format;
    ElemGraphicsBlendOperation BlendOperation;
    ElemGraphicsBlendFactor SourceBlendFactor;
    ElemGraphicsBlendFactor DestinationBlendFactor;
    ElemGraphicsBlendOperation BlendOperationAlpha;
    ElemGraphicsBlendFactor SourceBlendFactorAlpha;
    ElemGraphicsBlendFactor DestinationBlendFactorAlpha;
} ElemGraphicsPipelineStateRenderTarget;

typedef struct
{
    ElemGraphicsFormat Format;
    bool DepthDisableWrite;
    ElemGraphicsCompareFunction DepthCompareFunction;
} ElemGraphicsPipelineStateDepthStencil;

/**
 * Represents a collection of texture formats.
 */
typedef struct
{
    // Pointer to an array of ElemGraphicsPipelineStateRenderTarget.
    ElemGraphicsPipelineStateRenderTarget* Items;
    // Number of items in the array.
    uint32_t Length;
} ElemGraphicsPipelineStateRenderTargetSpan;

/**
 * Parameters for creating a graphics pipeline state.
 */
typedef struct
{
    // Shader library containing the shaders.
    ElemShaderLibrary ShaderLibrary;
    // Function name of the mesh shader in the shader library.
    const char* MeshShaderFunction;
    // Function name of the pixel shader in the shader library.
    const char* PixelShaderFunction;
    ElemGraphicsPipelineStateRenderTargetSpan RenderTargets;
    ElemGraphicsPipelineStateDepthStencil DepthStencil;
    ElemGraphicsFillMode FillMode;
    ElemGraphicsCullMode CullMode;
    // Optional debug name for the pipeline state.
    const char* DebugName;
} ElemGraphicsPipelineStateParameters;

/**
 * Parameters for creating a compute pipeline state.
 */
typedef struct
{
    // Shader library containing the shaders.
    ElemShaderLibrary ShaderLibrary;
    // Function name of the mesh shader in the shader library.
    const char* ComputeShaderFunction;
    // Optional debug name for the pipeline state.
    const char* DebugName;
} ElemComputePipelineStateParameters;

typedef struct
{
    ElemGraphicsResourceBarrierSyncType BeforeSync;
    ElemGraphicsResourceBarrierSyncType AfterSync;
    ElemGraphicsResourceBarrierAccessType BeforeAccess;
    ElemGraphicsResourceBarrierAccessType AfterAccess;
    ElemGraphicsResourceBarrierLayoutType BeforeLayout;
    ElemGraphicsResourceBarrierLayoutType AfterLayout;
} ElemGraphicsResourceBarrierOptions;

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

typedef struct
{
    // X coordinate of the rectangle's top left corner.
    float X;
    // Y coordinate of the rectangle's top left corner.
    float Y;
    // Width of the rectangle.
    float Width;
    // Height of the rectangle.
    float Height;
} ElemRectangle;

/**
 * Represents a collection of rectangles.
 */
typedef struct
{
    // Pointer to an array of ElemRectangle.
    ElemRectangle* Items;
    // Number of items in the array.
    uint32_t Length;
} ElemRectangleSpan;

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
    ElemGraphicsResource RenderTarget;
    // Color to clear the render target with if the load action is clear.
    ElemColor ClearColor;
    // Action to take when loading data into the render target at the beginning of a render pass.
    ElemRenderPassLoadAction LoadAction;
    // Action to take when storing data from the render target at the end of a render pass.
    ElemRenderPassStoreAction StoreAction;
} ElemRenderPassRenderTarget;

typedef struct
{
    ElemGraphicsResource DepthStencil;
    // TODO: Specify read or write mode ? (write by default)
    float DepthClearValue;
    ElemRenderPassLoadAction DepthLoadAction;
    ElemRenderPassStoreAction DepthStoreAction;
} ElemRenderPassDepthBufferStencil;

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
    ElemRenderPassDepthBufferStencil DepthStencil;
    // Viewports to be used in the render pass.
    ElemViewportSpan Viewports;

    ElemRectangleSpan ScissorRectangles;
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
// TODO: Add IsHdrSupported
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

ElemAPI ElemIOCommandQueue ElemCreateIOCommandQueue(const ElemIOCommandQueueOptions* options);
ElemAPI void ElemFreeIOCommandQueue(ElemIOCommandQueue ioQueue);
ElemAPI void ElemEnqueueIOCommand(ElemIOCommandQueue ioCommandQueue, const ElemIOCommandParameters* parameters);
ElemAPI ElemFence ElemSubmitIOCommandQueue();

ElemAPI ElemFence ElemExecuteIOCommands();

/**
 * Waits for a fence to reach its signaled state on the CPU, effectively synchronizing CPU and GPU operations.
 * @param fence The fence to wait on.
 */
ElemAPI void ElemWaitForFenceOnCpu(ElemFence fence);
ElemAPI bool ElemIsFenceCompleted(ElemFence fence);

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

ElemAPI ElemGraphicsHeap ElemCreateGraphicsHeap(ElemGraphicsDevice graphicsDevice, uint64_t sizeInBytes, const ElemGraphicsHeapOptions* options);
ElemAPI void ElemFreeGraphicsHeap(ElemGraphicsHeap graphicsHeap);

ElemAPI ElemGraphicsResourceInfo ElemCreateGraphicsBufferResourceInfo(ElemGraphicsDevice graphicsDevice, uint32_t sizeInBytes, ElemGraphicsResourceUsage usage, const ElemGraphicsResourceInfoOptions* options);
ElemAPI ElemGraphicsResourceInfo ElemCreateTexture2DResourceInfo(ElemGraphicsDevice graphicsDevice, uint32_t width, uint32_t height, uint32_t mipLevels, ElemGraphicsFormat format, ElemGraphicsResourceUsage usage, const ElemGraphicsResourceInfoOptions* options);

ElemAPI ElemGraphicsResource ElemCreateGraphicsResource(ElemGraphicsHeap graphicsHeap, uint64_t graphicsHeapOffset, const ElemGraphicsResourceInfo* resourceInfo);
ElemAPI void ElemFreeGraphicsResource(ElemGraphicsResource resource, const ElemFreeGraphicsResourceOptions* options);
ElemAPI ElemGraphicsResourceInfo ElemGetGraphicsResourceInfo(ElemGraphicsResource resource);
ElemAPI ElemDataSpan ElemGetGraphicsResourceDataSpan(ElemGraphicsResource resource);

ElemAPI ElemGraphicsResourceDescriptor ElemCreateGraphicsResourceDescriptor(ElemGraphicsResource resource, ElemGraphicsResourceDescriptorUsage usage, const ElemGraphicsResourceDescriptorOptions* options);
ElemAPI ElemGraphicsResourceDescriptorInfo ElemGetGraphicsResourceDescriptorInfo(ElemGraphicsResourceDescriptor descriptor);
ElemAPI void ElemFreeGraphicsResourceDescriptor(ElemGraphicsResourceDescriptor descriptor, const ElemFreeGraphicsResourceDescriptorOptions* options);

ElemAPI void ElemProcessGraphicsResourceDeleteQueue(void);

/**
 * Creates a shader library from provided binary data, allowing shaders to be loaded and used by graphics pipeline states.
 * @param graphicsDevice The device on which to create the shader library.
 * @param shaderLibraryData The binary data containing the shaders.
 * @return A handle to the newly created shader library.
 */
ElemAPI ElemShaderLibrary ElemCreateShaderLibrary(ElemGraphicsDevice graphicsDevice, ElemDataSpan shaderLibraryData);

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

ElemAPI ElemPipelineState ElemCompileComputePipelineState(ElemGraphicsDevice graphicsDevice, const ElemComputePipelineStateParameters* parameters);

/**
 * Releases resources associated with a pipeline state.
 * @param pipelineState The pipeline state to free.
 */
// TODO: Add the options with a fence???
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
ElemAPI void ElemPushPipelineStateConstants(ElemCommandList commandList, uint32_t offsetInBytes, ElemDataSpan data);

ElemAPI void ElemGraphicsResourceBarrier(ElemCommandList commandList, ElemGraphicsResourceDescriptor descriptor, const ElemGraphicsResourceBarrierOptions* options);

/**
 * Dispatches a compute operation on a command list, specifying the number of thread groups in each dimension.
 * @param commandList The command list on which the mesh operation is to be dispatched.
 * @param threadGroupCountX The number of thread groups in the X dimension.
 * @param threadGroupCountY The number of thread groups in the Y dimension.
 * @param threadGroupCountZ The number of thread groups in the Z dimension.
 */
ElemAPI void ElemDispatchCompute(ElemCommandList commandList, uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ);

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

ElemAPI void ElemSetScissorRectangle(ElemCommandList commandList, const ElemRectangle* rectangle);

ElemAPI void ElemSetScissorRectangles(ElemCommandList commandList, ElemRectangleSpan rectangles);

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
    ElemInputDeviceType_Unknown = 0,
    ElemInputDeviceType_Keyboard = 1,
    ElemInputDeviceType_Mouse = 2,
    ElemInputDeviceType_Gamepad = 3,
    ElemInputDeviceType_Touch= 4,
} ElemInputDeviceType;

typedef enum
{
    ElemInputType_Digital = 0,
    ElemInputType_Analog = 1,
    ElemInputType_Delta = 2,
    ElemInputType_Absolute = 3
} ElemInputType;

typedef enum
{
    ElemInputId_Unknown = 0,
    ElemInputId_KeyTilde = 1,
    ElemInputId_Key1 = 2,
    ElemInputId_Key2 = 3,
    ElemInputId_Key3 = 4,
    ElemInputId_Key4 = 5,
    ElemInputId_Key5 = 6,
    ElemInputId_Key6 = 7,
    ElemInputId_Key7 = 8,
    ElemInputId_Key8 = 9,
    ElemInputId_Key9 = 10,
    ElemInputId_Key0 = 11,
    ElemInputId_KeyDash = 12,
    ElemInputId_KeyEquals = 13,
    ElemInputId_KeyBackspace = 14,
    ElemInputId_KeyTab = 15,
    ElemInputId_KeyQ = 16,
    ElemInputId_KeyW = 17,
    ElemInputId_KeyE = 18,
    ElemInputId_KeyR = 19,
    ElemInputId_KeyT = 20,
    ElemInputId_KeyY = 21,
    ElemInputId_KeyU = 22,
    ElemInputId_KeyI = 23,
    ElemInputId_KeyO = 24,
    ElemInputId_KeyP = 25,
    ElemInputId_KeyLeftBrace = 26,
    ElemInputId_KeyRightBrace = 27,
    ElemInputId_KeyBackSlash = 28,
    ElemInputId_KeyCapsLock = 29,
    ElemInputId_KeyA = 30,
    ElemInputId_KeyS = 31,
    ElemInputId_KeyD = 32,
    ElemInputId_KeyF = 33,
    ElemInputId_KeyG = 34,
    ElemInputId_KeyH = 35,
    ElemInputId_KeyJ = 36,
    ElemInputId_KeyK = 37,
    ElemInputId_KeyL = 38,
    ElemInputId_KeySemiColon = 39,
    ElemInputId_KeyApostrophe = 40,
    ElemInputId_KeyEnter = 41,
    ElemInputId_KeyLeftShift = 42,
    ElemInputId_KeyZ = 43,
    ElemInputId_KeyX = 44,
    ElemInputId_KeyC = 45,
    ElemInputId_KeyV = 46,
    ElemInputId_KeyB = 47,
    ElemInputId_KeyN = 48,
    ElemInputId_KeyM = 49,
    ElemInputId_KeyComma = 50,
    ElemInputId_KeyPeriod = 51,
    ElemInputId_KeySlash = 52,
    ElemInputId_KeyRightShift = 53,
    ElemInputId_KeyLeftControl = 54,
    ElemInputId_KeyLeftAlt = 55,
    ElemInputId_KeySpacebar = 56,
    ElemInputId_KeyRightAlt = 57,
    ElemInputId_KeyRightControl = 58,
    ElemInputId_KeyInsert = 59,
    ElemInputId_KeyDelete = 60,
    ElemInputId_KeyLeftArrow = 61,
    ElemInputId_KeyHome = 62,
    ElemInputId_KeyEnd = 63,
    ElemInputId_KeyUpArrow = 64,
    ElemInputId_KeyDownArrow = 65,
    ElemInputId_KeyPageUp = 66,
    ElemInputId_KeyPageDown = 67,
    ElemInputId_KeyRightArrow = 68,
    ElemInputId_KeyNumpadLock = 69,
    ElemInputId_KeyNumpad7 = 70,
    ElemInputId_KeyNumpad4 = 71,
    ElemInputId_KeyNumpad1 = 72,
    ElemInputId_KeyNumpadDivide = 73,
    ElemInputId_KeyNumpad8 = 74,
    ElemInputId_KeyNumpad5 = 75,
    ElemInputId_KeyNumpad2 = 76,
    ElemInputId_KeyNumpad0 = 77,
    ElemInputId_KeyNumpadMultiply = 78,
    ElemInputId_KeyNumpad9 = 79,
    ElemInputId_KeyNumpad6 = 80,
    ElemInputId_KeyNumpad3 = 81,
    ElemInputId_KeyNumpadSeparator = 82,
    ElemInputId_KeyNumpadMinus = 83,
    ElemInputId_KeyNumpadAdd = 84,
    ElemInputId_KeyNumpadEnter = 85,
    ElemInputId_KeyEscape = 86,
    ElemInputId_KeyF1 = 87,
    ElemInputId_KeyF2 = 88,
    ElemInputId_KeyF3 = 89,
    ElemInputId_KeyF4 = 90,
    ElemInputId_KeyF5 = 91,
    ElemInputId_KeyF6 = 92,
    ElemInputId_KeyF7 = 93,
    ElemInputId_KeyF8 = 94,
    ElemInputId_KeyF9 = 95,
    ElemInputId_KeyF10 = 96,
    ElemInputId_KeyF11 = 97,
    ElemInputId_KeyF12 = 98,
    ElemInputId_KeyPrintScreen = 99,
    ElemInputId_KeyScrollLock = 100,
    ElemInputId_KeyPause = 101,
    ElemInputId_KeyLeftSystem = 102,
    ElemInputId_KeyRightSystem = 103,
    ElemInputId_KeyApp = 104,
    ElemInputId_MouseLeftButton = 105,
    ElemInputId_MouseRightButton = 106,
    ElemInputId_MouseMiddleButton = 107,
    ElemInputId_MouseExtraButton1 = 108,
    ElemInputId_MouseExtraButton2 = 109,
    ElemInputId_MouseAxisXNegative = 110,
    ElemInputId_MouseAxisXPositive = 111,
    ElemInputId_MouseAxisYNegative = 112,
    ElemInputId_MouseAxisYPositive = 113,
    ElemInputId_MouseWheelNegative = 114,
    ElemInputId_MouseWheelPositive = 115,
    ElemInputId_MouseHorizontalWheelNegative = 116,
    ElemInputId_MouseHorizontalWheelPositive = 117,
    ElemInputID_GamepadButtonA = 118,
    ElemInputID_GamepadButtonB = 119,
    ElemInputID_GamepadButtonX = 120,
    ElemInputID_GamepadButtonY = 121,
    ElemInputID_GamepadButtonMenu = 122,
    ElemInputID_GamepadButtonOptions = 123,
    ElemInputID_GamepadButtonHome = 124,
    ElemInputID_GamepadLeftShoulder = 125,
    ElemInputID_GamepadRightShoulder = 126,
    ElemInputID_GamepadLeftTrigger = 127,
    ElemInputID_GamepadRightTrigger = 128,
    ElemInputId_GamepadLeftStickXNegative = 129,
    ElemInputId_GamepadLeftStickXPositive = 130,
    ElemInputId_GamepadLeftStickYNegative = 131,
    ElemInputId_GamepadLeftStickYPositive = 132,
    ElemInputId_GamepadLeftStickButton = 133,
    ElemInputId_GamepadRightStickXNegative = 134,
    ElemInputId_GamepadRightStickXPositive = 135,
    ElemInputId_GamepadRightStickYNegative = 136,
    ElemInputId_GamepadRightStickYPositive = 137,
    ElemInputId_GamepadRightStickButton = 138,
    ElemInputId_GamepadDpadUp = 139,
    ElemInputId_GamepadDpadRight = 140,
    ElemInputId_GamepadDpadDown = 141,
    ElemInputId_GamepadDpadLeft = 142,
    ElemInputId_Touch = 143,
    ElemInputId_TouchXNegative = 144,
    ElemInputId_TouchXPositive = 145,
    ElemInputId_TouchYNegative = 146,
    ElemInputId_TouchYPositive = 147,
    ElemInputId_TouchXAbsolutePosition = 148,
    ElemInputId_TouchYAbsolutePosition = 149
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
    uint32_t InputDeviceTypeIndex;
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
