#include "ShaderCompiler.cpp"
#include "ShaderCompilerUtils.cpp"
#include "DirectXShaderCompiler.cpp"

#ifndef __linux__
#include "MetalShaderConverter.cpp"
#endif

#include "SystemFunctions.cpp"
#include "SystemDictionary.cpp"
#include "SystemMemory.cpp"
#include "SystemPlatformFunctions.cpp"

#ifndef _WIN32
#include "PosixPlatformFunctions.cpp"
#endif
