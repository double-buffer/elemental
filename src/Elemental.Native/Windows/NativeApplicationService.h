#pragma once

#include "SystemLogging.h"
#include "SystemMemory.h"
#include "Win32Application.h"
#include "Win32Window.h"

#pragma warning(disable: 4191)
#include "Libs/Win32DarkMode/DarkMode.h"
#pragma warning(default: 4191)

// HACK: Remove that
#include <map>

void ProcessMessages(Win32Application* application);
LRESULT CALLBACK Win32WindowCallBack(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

