#pragma once

#include <windows.h>
#include <wrl/client.h>
using namespace Microsoft::WRL;

#ifdef _DEBUG
#undef _DEBUG
#define _WASDEBUG
#endif

#define ElemToolsAPI extern "C" __declspec(dllexport)
#define AssertIfFailed(expression) SystemAssert(SUCCEEDED((expression)))
#define AssertIfFailedReturnNullHandle(expression) SystemAssertReturnNullHandle(SUCCEEDED((expression)))

#define popen _popen
#define pclose _pclose
#undef min

// TODO: Cleanup includes
#include <ctype.h>
#include <assert.h>
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

