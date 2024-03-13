#include "ElementalTools.h"

#include "SystemFunctions.cpp"

// TODO: Remove AssertIfFailed, only use assert is some specific places
#define AssertIfFailed(result) result

#include "DirectXShaderCompilerProvider.cpp"
#include "SpirvCrossShaderCompilerProvider.cpp"
#include "MetalShaderCompilerProvider.cpp"
#include "ShaderCompiler.cpp"

#include "SystemDictionary.cpp"
#include "SystemPlatformFunctions.cpp"
#include "SystemLogging.cpp"
#include "SystemMemory.cpp"
