#pragma once

#include "ElementalTools.h"
#include "SystemMemory.h"

bool MetalShaderConverterIsInstalled();
ElemShaderCompilationResult MetalShaderConverterCompileShader(MemoryArena memoryArena, ReadOnlySpan<uint8_t> shaderCode, ElemShaderLanguage targetLanguage, ElemToolsGraphicsApi targetGraphicsApi, const ElemCompileShaderOptions* options);
