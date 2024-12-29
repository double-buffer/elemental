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
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <libgen.h>
#include <dlfcn.h>

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

#include "meshoptimizer.h"
#include "fast_obj.h"
#include "cgltf.h"
#include "stb_image.h"
#include "stb_image_resize2.h"

#ifdef _WASDEBUG
#define _DEBUG
#endif

