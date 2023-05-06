#pragma once

#pragma warning(disable: 4820)
#pragma warning(disable: 4668)
#pragma warning(disable: 4365)
#pragma warning(disable: 4191)

// Inlining informational warnings
#pragma warning(disable: 4710)
#pragma warning(disable: 4711) 

#include <stdint.h>
#include <sys/stat.h>

#include <locale>
#include <codecvt>

#include <assert.h>
#include <io.h>


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

#include <initguid.h>
#include <setupapi.h>
#include <hidsdi.h>
#include <hidclass.h>
#include <cfgmgr32.h>
#include <dbt.h>

#pragma warning(default: 4820)
#pragma warning(default: 4668)
#pragma warning(default: 4365)
#pragma warning(default: 4191)

// TODO: should we use OneCoreUAP.lib ?
#pragma comment(lib, "hid.lib")
#pragma comment(lib, "Cfgmgr32.lib")
#pragma comment(lib, "SetupAPI") 

#include "SystemFunctions.h"