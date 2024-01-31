#pragma once

#include "ElementalTools.h"
#include "SystemFunctions.h"

#include "ShaderCompilerProvider.h"
#include "DirectXShaderCompilerProvider.h"
#include "SpirvCrossShaderCompilerProvider.h"
#include "MetalShaderCompilerProvider.h"

const ShaderCompilerResult CreateErrorResult(ShaderStage stage, const uint8_t* entryPoint, const uint8_t* message);
