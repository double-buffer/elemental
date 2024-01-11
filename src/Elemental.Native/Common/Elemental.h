#pragma once
#include "ElementalCommon.h"

typedef struct 
{
    LogMessageHandlerPtr LogMessageHandler;
} NativeApplicationOptions;

typedef enum
{
    None = 0,
    Active = 1,
    Closing = 2
} NativeApplicationStatusFlags;

typedef struct
{
    uint32_t Status;
} NativeApplicationStatus;

typedef bool (*RunHandlerPtr)(NativeApplicationStatus status);

typedef enum
{
    NativeWindowState_Normal = 0,
    NativeWindowState_Minimized = 1,
    NativeWindowState_Maximized = 2,
    NativeWindowState_FullScreen = 3
} NativeWindowState;

typedef struct
{
    wchar_t* Title;
    uint32_t Width;
    uint32_t Height;
    NativeWindowState WindowState;
} NativeWindowOptions;

typedef struct
{
    uint32_t Width;
    uint32_t Height;
    float UIScale;
    NativeWindowState WindowState;
} NativeWindowSize;

typedef struct
{
    char* DeviceName;
    GraphicsApi GraphicsApi;
    uint64_t DeviceId;
    uint64_t AvailableMemory;
} GraphicsDeviceInfo;

typedef enum
{
    GraphicsDiagnostics_None = 0,
    GraphicsDiagnostics_Debug = 1
} GraphicsDiagnostics;

typedef struct
{
    GraphicsDiagnostics GraphicsDiagnostics;
} GraphicsServiceOptions;

typedef struct
{
    uint64_t DeviceId;
} GraphicsDeviceOptions;

typedef enum
{
    CommandQueueType_Graphics = 0,
    CommandQueueType_Compute = 1,
    CommandQueueType_Copy = 2
} CommandQueueType;

typedef struct
{
    void* CommandQueuePointer;
    uint64_t FenceValue;
} Fence;

typedef enum
{
    TextureFormat_Unknown = 0,
    TextureFormat_Rgba8UnormSrgb = 1
} TextureFormat;

typedef enum
{
    SwapChainFormat_Default = 0,
    SwapChainFormat_HighDynamicRange = 1
} SwapChainFormat;

typedef struct
{
    uint32_t Width;
    uint32_t Height;
    SwapChainFormat Format;
    uint32_t MaximumFrameLatency;
} SwapChainOptions;

typedef struct
{
    ShaderStage Stage;
    uint8_t* EntryPoint;
    void* DataPointer;
    uint32_t DataCount;
    ShaderMetaData* MetaDataPointer;
    uint32_t MetaDataCount;
} ShaderPart;

typedef struct
{
    void* TexturePointer;
    NullableVector4 ClearColor;
} RenderPassRenderTarget;

typedef struct
{
    bool HasValue;
    RenderPassRenderTarget Value;
} NullableRenderPassRenderTarget;

typedef struct
{
    NullableRenderPassRenderTarget RenderTarget0;
    NullableRenderPassRenderTarget RenderTarget1;
    NullableRenderPassRenderTarget RenderTarget2;
    NullableRenderPassRenderTarget RenderTarget3;
} RenderPassDescriptor;

typedef struct
{
    void* DataPointer;
    size_t DataSize;
    void* InputObjectsPointer;
    size_t InputObjectsSize;
} InputState;

typedef enum : uint8_t
{
    Gamepad1LeftStickX,
    Gamepad1LeftStickY,
    Gamepad1RightStickX,
    Gamepad1RightStickY,
    Gamepad1DpadUp,
    Gamepad1DpadRight,
    Gamepad1DpadDown,
    Gamepad1DpadLeft,
    Gamepad1Button1,
    Gamepad1Button2,
    Gamepad1Button3,
    Gamepad1Button4,
    Gamepad1LeftShoulder,
    Gamepad1RightShoulder,
    InputObjectKey_MaxValue = Gamepad1RightShoulder
} InputObjectKey;

typedef enum : uint8_t
{
    GamepadLeftStickX,
    GamepadLeftStickY,
    GamepadRightStickX,
    GamepadRightStickY,
    GamepadDpadUp,
    GamepadDpadRight,
    GamepadDpadDown,
    GamepadDpadLeft,
    GamepadButton1,
    GamepadButton2,
    GamepadButton3,
    GamepadButton4,
    GamepadLeftShoulder,
    GamepadRightShoulder
} InputsValueId;

typedef enum : uint8_t
{
    InputObjectType_Digital,
    InputObjectType_Analog
} InputObjectType;

typedef struct
{
    uint32_t Offset;
    uint32_t BitPosition;
} InputObjectValueAddress;

typedef struct
{
    InputObjectType Type;
    InputObjectValueAddress Value;
    InputObjectValueAddress PreviousValue;
} InputObject;

typedef struct
{
    uint32_t DeviceId;
    InputsValueId Id;
    uint64_t Timestamp;
    float Value;
} InputsValue;
