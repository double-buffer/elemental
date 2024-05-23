#pragma once

#include <xxh3.h>

#include <dlfcn.h>
#include <fcntl.h>
#include <libgen.h>
#include <pthread.h>
#include <stdarg.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include <linux/input-event-codes.h>

#include <wayland-client.h>
#include <relative-pointer-unstable-v1-client-protocol.h>
#include <xdg-shell-protocol.h>

typedef signed int HRESULT;
#define SUCCEEDED(Status) ((HRESULT)(Status) >= 0)
#define ElemAPI extern "C" __attribute__((visibility("default")))
#define PackedStruct struct __attribute__((__packed__))
#define AssertIfFailed(expression) SystemAssert(SUCCEEDED((expression)))
#define AssertIfFailedReturnNullHandle(expression) SystemAssertReturnNullHandle(SUCCEEDED((expression)))
#define ARRAYSIZE(a) (sizeof(a) / sizeof(*(a)))

