#pragma once

#include <xxh3.h>

// Include mach.h?
#include <Carbon/Carbon.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>

#define NS_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#include <Metal/Metal.hpp>
#include "AppKit/AppKit.hpp"
#include <IOKit/hid/IOHIDManager.h>

#define ElemAPI extern "C" __attribute__((visibility("default")))
#define PackedStruct struct __attribute__((__packed__))
#define AssertIfFailed(expression) SystemAssert(SUCCEEDED((expression)))
#define AssertIfFailedReturnNullHandle(expression) SystemAssertReturnNullHandle(SUCCEEDED((expression)))
#define ARRAYSIZE(a) (sizeof(a) / sizeof(*(a)))
