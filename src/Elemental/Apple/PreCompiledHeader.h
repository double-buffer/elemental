#pragma once

#include <xxh3.h>

// Include mach.h?
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/mman.h>

#define NS_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#define MTK_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#include <Metal/Metal.hpp>

#if defined(TARGET_OS_OSX) && TARGET_OS_OSX
#include "Frameworks/AppKit/AppKit.hpp"
#else
#define UI_PRIVATE_IMPLEMENTATION
#include "Frameworks/UIKit/UIKit.hpp"
#endif

#include "Frameworks/Foundation/Foundation.hpp"
#include "Frameworks/QuartzCore/QuartzCore.hpp"

typedef signed int HRESULT;
#define SUCCEEDED(Status) ((HRESULT)(Status) >= 0)
#define ElemAPI extern "C" __attribute__((visibility("default")))
#define PackedStruct struct __attribute__((__packed__))
#define AssertIfFailed(expression) SystemAssert(SUCCEEDED((expression)))
#define AssertIfFailedReturnNullHandle(expression) SystemAssertReturnNullHandle(SUCCEEDED((expression)))
#define ARRAYSIZE(a) (sizeof(a) / sizeof(*(a)))
