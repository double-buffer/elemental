#pragma once

#include "ElementalTools.h"
#include "SystemMemory.h"

bool DirectXShaderCompilerIsInstalled();
ElemShaderCompilationResult DirectXShaderCompilerCompileShader(ReadOnlySpan<uint8_t> shaderCode, ElemShaderLanguage targetLanguage, const ElemCompileShaderOptions* options);
