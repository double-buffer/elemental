#pragma once

#include "ElementalTools.h"
#include "SystemMemory.h"
#include "SystemSpan.h"

ReadOnlySpan<uint8_t> LoadFileData(const char* path);
void ResetLoadFileDataMemory();

ElemToolsMessageSpan ConstructErrorMessageSpan(MemoryArena memoryArena, const char* errorMessage);
