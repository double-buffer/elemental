#pragma once

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

#include "../Platforms/Common/SystemFunctions.h"

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