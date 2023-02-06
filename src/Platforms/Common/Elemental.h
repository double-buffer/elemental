#pragma once
#include <stdbool.h>
#include <stdint.h>

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
    GraphicsDiagnostics_None,
    GraphicsDiagnostics_Debug
};

struct GraphicsDeviceOptions
{
    enum GraphicsDiagnostics GraphicsDiagnostics;
    uint64_t DeviceId;
};