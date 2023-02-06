#pragma once
#include <stdbool.h>

enum NativeApplicationStatusFlags
{
    None = 0,
    Active = 1,
    Closing = 2
};

struct NativeApplicationStatus
{
    unsigned int Status;
};

typedef bool (*RunHandlerPtr)(struct NativeApplicationStatus status);

enum NativeWindowState
{
    Normal,
    Maximized
};

struct NativeWindowOptions
{
    unsigned char* Title;
    int Width;
    int Height;
    enum NativeWindowState WindowState;
};

struct NativeWindowSize
{
    int Width;
    int Height;
    float UIScale;
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
    unsigned char* DeviceName;
    enum GraphicsApi GraphicsApi;
    unsigned long DeviceId;
    unsigned long AvailableMemory;
};

enum GraphicsDiagnostics
{
    GraphicsDiagnostics_None,
    GraphicsDiagnostics_Debug
};

struct GraphicsDeviceOptions
{
    enum GraphicsDiagnostics GraphicsDiagnostics;
    unsigned long DeviceId;
};