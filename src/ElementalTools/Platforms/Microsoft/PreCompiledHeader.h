#pragma once
#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX

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

#include "metal_irconverter/metal_irconverter.h"

#include "meshoptimizer.h"
#include "fast_obj.h"
#include "cgltf.h"
#include "stb_image.h"
#include "stb_image_resize2.h"
#include "mikktspace.h"

#ifdef _WASDEBUG
#define _DEBUG
#endif

