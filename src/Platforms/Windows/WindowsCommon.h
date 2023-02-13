#pragma once
#include <stdint.h>
#include <sys/stat.h>

#include <locale>
#include <codecvt>
#include <string>

#include <assert.h>

#define D3D12SDKVersion 608
#define D3D12SDKPath u8".\\D3D12\\"

#include <ShellScalingAPI.h>
#include <Windows.h>
#include <dwmapi.h>
#include <Uxtheme.h>

#include <wrl/client.h>
#include "d3d12.h"
#include <d3dcompiler.h>

#if defined(NTDDI_WIN10_RS2)
#include <dxgi1_6.h>
#include <dxgidebug.h>
#else
#include <dxgi1_5.h>
#endif


#define AssertIfFailed(result) assert(!FAILED(result))

#define DllExport extern "C" __declspec(dllexport)

struct WindowsEvent
{
    HWND Window;
    UINT Message;
    WPARAM WParam;
    LPARAM LParam;
};

using namespace Microsoft::WRL;