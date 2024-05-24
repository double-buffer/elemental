#pragma once

#include <xxh3.h>

// TODO: Review headers
#include <stdio.h>
#include <dirent.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <poll.h>
#include <pthread.h>
#include <stdarg.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <linux/input-event-codes.h>
#include <linux/hidraw.h>
#include <libudev.h>

#include <wayland-client.h>
#include <wayland-cursor.h>
#include <relative-pointer-unstable-v1-client-protocol.h>
#include <libdecor.h>


typedef signed int HRESULT;
#define SUCCEEDED(Status) ((HRESULT)(Status) >= 0)
#define ElemAPI extern "C" __attribute__((visibility("default")))
#define PackedStruct struct __attribute__((__packed__))
#define AssertIfFailed(expression) SystemAssert(SUCCEEDED((expression)))
#define AssertIfFailedReturnNullHandle(expression) SystemAssertReturnNullHandle(SUCCEEDED((expression)))
#define ARRAYSIZE(a) (sizeof(a) / sizeof(*(a)))

