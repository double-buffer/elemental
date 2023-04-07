#pragma once

#pragma warning(disable: 4820)
#pragma warning(disable: 4668)
#pragma warning(disable: 4365)
#pragma warning(disable: 5026)
#pragma warning(disable: 4625)
#pragma warning(disable: 4626)
#pragma warning(disable: 5027)
#pragma warning(disable: 4061)
#pragma warning(disable: 5204)
#pragma warning(disable: 5045)

#ifdef _DEBUG
#undef _DEBUG
#define _WASDEBUG
#endif

#if _WINDOWS
#define DllExport extern "C" __declspec(dllexport)
#include <windows.h>
#include <wrl/client.h>
using namespace Microsoft::WRL;

#define popen _popen
#define pclose _pclose
#undef min
#else
#define DllExport extern "C" __attribute__((visibility("default"))) 
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

#define AssertIfFailed(result) assert(!FAILED(result))

template<typename T>
struct Span
{
    Span()
    {
        Pointer = nullptr;
        Length = 0;
    }

    Span(T* pointer, uint32_t length) : Pointer(pointer), Length(length)
    {
    }

    T* Pointer;
    uint32_t Length;

    bool IsEmpty()
    {
        return Length == 0;
    }

    static Span<T> Empty()
    {
        return Span<T>();
    }
};

#pragma warning(default: 4820)
#pragma warning(default: 4668)
#pragma warning(default: 4365)
#pragma warning(default: 5026)
#pragma warning(default: 4625)
#pragma warning(default: 4626)
#pragma warning(default: 5027)
#pragma warning(default: 4061)
#pragma warning(default: 5204)
#pragma warning(default: 5045)