cmake_minimum_required(VERSION 3.26)
project(elemental_tools_native LANGUAGES CXX)

add_library(Elemental.Tools.Native SHARED UnityBuild.cpp)

target_compile_options(Elemental.Tools.Native PRIVATE -Wall)
target_compile_features(Elemental.Tools.Native PRIVATE cxx_std_20)
target_precompile_headers(Elemental.Tools.Native PRIVATE PrecompiledHeader.h)
target_include_directories(Elemental.Tools.Native PRIVATE ../Platforms/Common/)
target_link_libraries(Elemental.Tools.Native PRIVATE spirv-cross-core spirv-cross-cpp spirv-cross-msl spirv-cross-glsl spirv-cross-reflect spirv-cross-util)

if(WIN32)
    target_compile_options(Elemental.Tools.Native PRIVATE -EHs -MD)
    target_compile_definitions(Elemental.Tools.Native PRIVATE UNICODE _UNICODE)
    
    target_include_directories(Elemental.Tools.Native PRIVATE ../../external/shader-compilers/dxc/inc/dxc/)
    target_include_directories(Elemental.Tools.Native PRIVATE ../../external/shader-compilers/spirv-cross/include/)
    target_link_directories(Elemental.Tools.Native PRIVATE ../../external/shader-compilers/spirv-cross/lib/lib/)
endif()