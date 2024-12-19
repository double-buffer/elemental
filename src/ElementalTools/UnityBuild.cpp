#include "ToolsUtils.cpp"
#include "Shaders/ShaderCompiler.cpp"
#include "Shaders/ShaderCompilerUtils.cpp"
#include "Shaders/DirectXShaderCompiler.cpp"

#ifndef __linux__
#include "Shaders/MetalShaderConverter.cpp"
#endif

#include "fast_obj.c"
#include "SceneLoading/SceneLoader.cpp"

#include "Meshes/MeshletBuilder.cpp"

#include "SystemFunctions.cpp"
#include "SystemDictionary.cpp"
#include "SystemMemory.cpp"
#include "SystemPlatformFunctions.cpp"

#ifndef _WIN32
#include "PosixPlatformFunctions.cpp"
#endif
