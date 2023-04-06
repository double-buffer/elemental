#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include "ElementalCommon.h"

enum NativeApplicationStatusFlags
{
    None = 0,
    Active = 1,
    Closing = 2
};

struct NativeApplicationStatus
{
    uint32_t Status;
};

typedef bool (*RunHandlerPtr)(struct NativeApplicationStatus status);

enum NativeWindowState
{
    NativeWindowState_Normal = 0,
    NativeWindowState_Minimized = 1,
    NativeWindowState_Maximized = 2,
    NativeWindowState_FullScreen = 3
};

struct NativeWindowOptions
{
    uint8_t* Title;
    uint32_t Width;
    uint32_t Height;
    enum NativeWindowState WindowState;
};

struct NativeWindowSize
{
    uint32_t Width;
    uint32_t Height;
    float_t UIScale;
    enum NativeWindowState WindowState;
};

struct GraphicsDeviceInfo
{
    const uint8_t* DeviceName;
    enum GraphicsApi GraphicsApi;
    uint64_t DeviceId;
    uint64_t AvailableMemory;
};

enum GraphicsDiagnostics
{
    GraphicsDiagnostics_None = 0,
    GraphicsDiagnostics_Debug = 1
};

struct GraphicsServiceOptions
{
    enum GraphicsDiagnostics GraphicsDiagnostics;
};

struct GraphicsDeviceOptions
{
    uint64_t DeviceId;
};

enum CommandQueueType
{
    CommandQueueType_Graphics = 0,
    CommandQueueType_Compute = 1,
    CommandQueueType_Copy = 2
};

struct Fence
{
    void* CommandQueuePointer;
    uint64_t FenceValue;
};

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

struct SwapChainOptions
{
    uint32_t Width;
    uint32_t Height;
    enum SwapChainFormat Format;
    uint32_t MaximumFrameLatency;
};

struct ShaderPart
{
    enum ShaderStage Stage;
    uint8_t* EntryPoint;
    void* DataPointer;
    uint32_t DataCount;
    struct ShaderMetaData* MetaDataPointer;
    uint32_t MetaDataCount;
};

struct RenderPassRenderTarget
{
    void* TexturePointer;
    struct NullableVector4 ClearColor;
};

struct NullableRenderPassRenderTarget
{
    bool HasValue;
    struct RenderPassRenderTarget Value;
};

struct RenderPassDescriptor
{
    struct NullableRenderPassRenderTarget RenderTarget0;
    struct NullableRenderPassRenderTarget RenderTarget1;
    struct NullableRenderPassRenderTarget RenderTarget2;
    struct NullableRenderPassRenderTarget RenderTarget3;
};

struct InputState
{
    void* DataPointer;
    size_t DataSize;
    void* InputObjectsPointer;
    size_t InputObjectsSize;
};

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

struct InputObjectValueAddress
{
    uint32_t Offset;
    uint32_t BitPosition;
};

struct InputObject
{
    enum InputObjectType Type;
    struct InputObjectValueAddress Value;
    struct InputObjectValueAddress PreviousValue;
};