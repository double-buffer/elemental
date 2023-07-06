#pragma once

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
#include <dxgi1_6.h>
#include <dxgidebug.h>

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

// TODO: should we use OneCoreUAP.lib ?
#pragma comment(lib, "hid.lib")
#pragma comment(lib, "Cfgmgr32.lib")
#pragma comment(lib, "SetupAPI") 





