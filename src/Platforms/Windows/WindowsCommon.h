#pragma once
#include <stdint.h>
#include <sys/stat.h>

#include <locale>
#include <codecvt>
#include <string>

#include <map>
#include <vector>
#include <stack>
#include <assert.h>

#define ISOLATION_AWARE_ENABLED 1

#include <ShellScalingAPI.h>
#include <Windows.h>
#include <dwmapi.h>
#include <Uxtheme.h>

#define AssertIfFailed(result) assert(!FAILED(result))

#define DllExport extern "C" __declspec(dllexport)

struct WindowsEvent
{
    HWND Window;
    UINT Message;
    WPARAM WParam;
    LPARAM LParam;
};