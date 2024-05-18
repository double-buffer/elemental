#pragma once

#include "Elemental.h"

void InitWin32Inputs(HWND window);
void ProcessWin32KeyInput(ElemWindow window, UINT message, WPARAM wParam, LPARAM lParam);
void ProcessWin32RawInput(ElemWindow window, LPARAM lParam);
void RemoveWin32InputDevice(HANDLE rawInputDevice);
