#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include "ElementalCommon.h"

// TODO: Convert to typedef

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

enum NativeWindowState
{
    NativeWindowState_Normal = 0,
    NativeWindowState_Minimized = 1,
    NativeWindowState_Maximized = 2,
    NativeWindowState_FullScreen = 3
};

typedef struct
{
    uint8_t* Title;
    uint32_t Width;
    uint32_t Height;
    enum NativeWindowState WindowState;
} NativeWindowOptions;

typedef struct
{
    uint32_t Width;
    uint32_t Height;
    float_t UIScale;
    enum NativeWindowState WindowState;
} NativeWindowSize;

typedef struct
{
    const uint8_t* DeviceName;
    enum GraphicsApi GraphicsApi;
    uint64_t DeviceId;
    uint64_t AvailableMemory;
} GraphicsDeviceInfo;

enum GraphicsDiagnostics
{
    GraphicsDiagnostics_None = 0,
    GraphicsDiagnostics_Debug = 1
};

typedef struct
{
    enum GraphicsDiagnostics GraphicsDiagnostics;
} GraphicsServiceOptions;

typedef struct
{
    uint64_t DeviceId;
} GraphicsDeviceOptions;

enum CommandQueueType
{
    CommandQueueType_Graphics = 0,
    CommandQueueType_Compute = 1,
    CommandQueueType_Copy = 2
};

typedef struct
{
    void* CommandQueuePointer;
    uint64_t FenceValue;
} Fence;

enum TextureFormat
{
    TextureFormat_Unknown = 0,
    TextureFormat_Rgba8UnormSrgb = 1
};

enum SwapChainFormat
{
    SwapChainFormat_Default = 0,
    SwapChainFormat_HighDynamicRange = 1
};

typedef struct
{
    uint32_t Width;
    uint32_t Height;
    enum SwapChainFormat Format;
    uint32_t MaximumFrameLatency;
} SwapChainOptions;

typedef struct
{
    enum ShaderStage Stage;
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

enum InputObjectKey : uint8_t
{
    Gamepad1LeftStickX,
    Gamepad1LeftStickY,
    Gamepad1RightStickX,
    Gamepad1RightStickY,
    Gamepad1Button1,
    Gamepad1Button2,
    InputObjectKey_MaxValue = Gamepad1Button2
};

enum InputObjectType : uint8_t
{
    InputObjectType_Digital,
    InputObjectType_Analog
};

typedef struct
{
    uint32_t Offset;
    uint32_t BitPosition;
} InputObjectValueAddress;

typedef struct
{
    enum InputObjectType Type;
    InputObjectValueAddress Value;
    InputObjectValueAddress PreviousValue;
} InputObject;