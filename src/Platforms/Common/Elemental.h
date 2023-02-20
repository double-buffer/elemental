#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <math.h>

struct Vector4
{
    float X, Y, Z, W;
};

struct NullableVector4
{
    bool HasValue;
    struct Vector4 Value;
};

struct NullablePointer
{
    bool HasValue;
    void* Value;
};

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
    Normal,
    Maximized
};

struct NativeWindowOptions
{
    uint8_t* Title;
    int32_t Width;
    int32_t Height;
    enum NativeWindowState WindowState;
};

struct NativeWindowSize
{
    int32_t Width;
    int32_t Height;
    float_t UIScale;
};

enum GraphicsApi
{
    GraphicsApi_Unknown = 0,
    GraphicsApi_Direct3D12 = 1,
    GraphicsApi_Vulkan = 2,
    GraphicsApi_Metal = 3
};

struct GraphicsDeviceInfo
{
    uint8_t* DeviceName;
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

struct SwapChainOptions
{
    int32_t Width;
    int32_t Height;
    enum TextureFormat TextureFormat;
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
};