#include "ToolsUtils.cpp"
#include "Shaders/ShaderCompiler.cpp"
#include "Shaders/ShaderCompilerUtils.cpp"
#include "Shaders/DirectXShaderCompiler.cpp"

#ifndef __linux__
#include "Shaders/MetalShaderConverter.cpp"
#endif

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"
#include "fast_obj.c"
#include "SceneLoading/SceneLoader.cpp"

#include "Meshes/MeshletBuilder.cpp"

#ifdef _WIN32
#define STBI_WINDOWS_UTF8
#endif

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize2.h"

#include "Textures/TextureLoader.cpp"
#include "Textures/TextureProcessing.cpp"

#include "SystemFunctions.cpp"
#include "SystemDictionary.cpp"
#include "SystemMemory.cpp"
#include "SystemPlatformFunctions.cpp"

#ifndef _WIN32
#include "PosixPlatformFunctions.cpp"
#endif
