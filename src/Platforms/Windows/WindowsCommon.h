#pragma once
#include <stdint.h>
#include <sys/stat.h>

#include <locale>
#include <codecvt>

#include <assert.h>
#include <io.h>

#define D3D12SDKVersion 608
#define D3D12SDKPath u8".\\"

#include <ShellScalingAPI.h>
#include <Windows.h>
#include <dwmapi.h>
#include <Uxtheme.h>

#include <wrl/client.h>
#include "d3d12.h"

#if defined(NTDDI_WIN10_RS2)
#include <dxgi1_6.h>
#include <dxgidebug.h>
#else
#include <dxgi1_5.h>
#endif

#ifdef _DEBUG
#define AssertIfFailed(result) assert(!FAILED(result))
#else
#define AssertIfFailed(result) result
#endif

#define DllExport extern "C" __declspec(dllexport)

struct WindowsEvent
{
    HWND Window;
    UINT Message;
    WPARAM WParam;
    LPARAM LParam;
};

using namespace Microsoft::WRL;

