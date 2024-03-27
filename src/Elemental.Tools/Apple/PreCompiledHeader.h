#pragma once

#ifdef _DEBUG
#undef _DEBUG
#define _WASDEBUG
#endif

#define ElemToolsAPI extern "C" __attribute__((visibility("default"))) 
#define AssertIfFailed(expression) SystemAssert(SUCCEEDED((expression)))
#define AssertIfFailedReturnNullHandle(expression) SystemAssertReturnNullHandle(SUCCEEDED((expression)))
#define ComPtr CComPtr
#undef _WIN32
#include <Carbon/Carbon.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>

#define NS_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#include "AppKit/AppKit.hpp"

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

#ifdef _WASDEBUG
#define _DEBUG
#endif

