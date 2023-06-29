#pragma once

#ifdef _DEBUG
#undef _DEBUG
#define _WASDEBUG
#endif

#ifdef _WINDOWS
#define DllExport extern "C" __declspec(dllexport)
#include <windows.h>
#include <wrl/client.h>
using namespace Microsoft::WRL;

#define popen _popen
#define pclose _pclose
#undef min
#else
#define ComPtr CComPtr
#undef _WIN32
#include <iconv.h>
#include <unistd.h>
#include <dlfcn.h>
#endif

// TODO: Cleanup includes
#include <ctype.h>
#include <assert.h>
#include <stdio.h>
#include <stdint.h>

#include <locale>
#include <codecvt>
#include <string>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <cstdlib>

#include "dxcapi.h"
#include "d3d12shader.h"

#include "spirv_msl.hpp"

#ifdef _WASDEBUG
#define _DEBUG
#endif

