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

struct GraphicsDeviceInfo
{
    unsigned char* DeviceName;
    unsigned char* GraphicsApiName;
    unsigned char* DriverVersion;
};

enum GraphicsDiagnostics
{
    GraphicsDiagnostics_None,
    GraphicsDiagnostics_Debug
};

struct GraphicsDeviceOptions
{
    enum GraphicsDiagnostics GraphicsDiagnostics;
};