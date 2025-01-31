#include "ToolsUtils.cpp"
#include "Shaders/ShaderCompiler.cpp"
#include "Shaders/ShaderCompilerUtils.cpp"
#include "Shaders/DirectXShaderCompiler.cpp"

#ifndef __linux__
#include "Shaders/MetalShaderConverter.cpp"
#endif

#include "SceneLoading/TangentSpaceGenerator.cpp"

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"
#include "fast_obj.c"
#include "SceneLoading/SceneLoader.cpp"

#include "Meshes/MeshletBuilder.cpp"

#ifdef _WIN32
#define STBI_WINDOWS_UTF8
#endif

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc99-extensions"
#pragma clang diagnostic ignored "-Wmacro-redefined"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize2.h"
#pragma clang diagnostic pop

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-but-set-variable"
#include "mikktspace.c"
#pragma clang diagnostic pop

#include "bc7enc.cpp"

#include "Textures/TextureLoader.cpp"
#include "Textures/TextureProcessing.cpp"

#include "SystemFunctions.cpp"
#include "SystemDictionary.cpp"
#include "SystemMemory.cpp"
#include "SystemPlatformFunctions.cpp"

#ifndef _WIN32
#include "PosixPlatformFunctions.cpp"
#endif
