#pragma once

#include "SystemSpan.h"

ReadOnlySpan<uint8_t> LoadFileData(const char* path);
void ResetLoadFileDataMemory();
