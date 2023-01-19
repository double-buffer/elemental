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

struct NativeWindowDescription
{
    unsigned char* Title;
    int Width;
    int Height;
    bool IsDpiAware;
    enum NativeWindowState WindowState;
};