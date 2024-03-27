#pragma once

#include "ElementalTools.h"
#include "SystemMemory.h"

bool MetalShaderConverterIsInstalled();
ElemShaderCompilationResult MetalShaderConverterCompileShader(ReadOnlySpan<uint8_t> shaderCode, ElemShaderLanguage targetLanguage, const ElemCompileShaderOptions* options);
