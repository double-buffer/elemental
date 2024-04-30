#pragma once

#include <xxh3.h>

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <libgen.h>
#include <dlfcn.h>

#include <gtk/gtk.h>
#include <gdk/wayland/gdkwayland.h>
#include <wayland-client.h>

typedef signed int HRESULT;
#define SUCCEEDED(Status) ((HRESULT)(Status) >= 0)
#define ElemAPI extern "C" __attribute__((visibility("default")))
#define PackedStruct struct __attribute__((__packed__))
#define AssertIfFailed(expression) SystemAssert(SUCCEEDED((expression)))
#define AssertIfFailedReturnNullHandle(expression) SystemAssertReturnNullHandle(SUCCEEDED((expression)))
#define ARRAYSIZE(a) (sizeof(a) / sizeof(*(a)))

