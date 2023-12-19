#pragma once

#include "SystemSpan.h"
#include "MacOSApplication.h"

void ProcessEvents(MacOSApplication* application);
void CreateApplicationMenu(ReadOnlySpan<char> applicationName);
