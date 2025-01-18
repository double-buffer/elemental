#=======================================================================
# DirectX Shader Compiler
#=======================================================================
if(NOT BUILD_FOR_IOS)
    add_subdirectory(./DirectXShaderCompiler)
endif()

#=======================================================================
# Metal Shader Converter
#=======================================================================
if(NOT BUILD_FOR_IOS)
    add_subdirectory(./MetalShaderConverter)
endif()

#=======================================================================
# Linux Wayland
#=======================================================================
if(LINUX)
    add_library(Wayland INTERFACE)

    find_package(PkgConfig REQUIRED)

    pkg_check_modules(WAYLAND REQUIRED wayland-client wayland-cursor wayland-protocols libudev libdecor-0)

    target_include_directories(Wayland INTERFACE ${WAYLAND_INCLUDE_DIRS})
    target_link_directories(Wayland INTERFACE ${WAYLAND_LIBRARY_DIRS})
    target_compile_options(Wayland INTERFACE ${WAYLAND_CFLAGS_OTHER})
    target_link_libraries(Wayland INTERFACE ${WAYLAND_LIBRARIES})

    # Set protocol directories
    set(STABLE_PROTOCOLS_DIR "/usr/share/wayland-protocols/stable")
    set(UNSTABLE_PROTOCOLS_DIR "/usr/share/wayland-protocols/unstable")
    set(GENERATED_DIR "${CMAKE_BINARY_DIR}/generated")
    file(MAKE_DIRECTORY ${GENERATED_DIR})

    add_custom_command(
        OUTPUT
            ${GENERATED_DIR}/xdg-shell-client-protocol.h
            ${GENERATED_DIR}/xdg-shell-protocol.c
        COMMAND wayland-scanner client-header
                ${STABLE_PROTOCOLS_DIR}/xdg-shell/xdg-shell.xml
                ${GENERATED_DIR}/xdg-shell-client-protocol.h
        COMMAND wayland-scanner private-code
                ${STABLE_PROTOCOLS_DIR}/xdg-shell/xdg-shell.xml
                ${GENERATED_DIR}/xdg-shell-protocol.c
        COMMENT "Generating xdg-shell protocol files"
    )

    # Generate relative-pointer protocol files
    add_custom_command(
        OUTPUT ${GENERATED_DIR}/relative-pointer-unstable-v1-client-protocol.h ${GENERATED_DIR}/relative-pointer-unstable-v1-client-protocol.c
        COMMAND wayland-scanner client-header ${UNSTABLE_PROTOCOLS_DIR}/relative-pointer/relative-pointer-unstable-v1.xml ${GENERATED_DIR}/relative-pointer-unstable-v1-client-protocol.h
        COMMAND wayland-scanner code ${UNSTABLE_PROTOCOLS_DIR}/relative-pointer/relative-pointer-unstable-v1.xml ${GENERATED_DIR}/relative-pointer-unstable-v1-client-protocol.c
        DEPENDS ${UNSTABLE_PROTOCOLS_DIR}/relative-pointer/relative-pointer-unstable-v1.xml
        COMMENT "Generating relative-pointer protocol files"
    )

    # Generate pointer-constraints protocol files
    add_custom_command(
        OUTPUT ${GENERATED_DIR}/pointer-constraints-unstable-v1-client-protocol.h ${GENERATED_DIR}/pointer-constraints-unstable-v1-client-protocol.c
        COMMAND wayland-scanner client-header ${UNSTABLE_PROTOCOLS_DIR}/pointer-constraints/pointer-constraints-unstable-v1.xml ${GENERATED_DIR}/pointer-constraints-unstable-v1-client-protocol.h
        COMMAND wayland-scanner code ${UNSTABLE_PROTOCOLS_DIR}/pointer-constraints/pointer-constraints-unstable-v1.xml ${GENERATED_DIR}/pointer-constraints-unstable-v1-client-protocol.c
        DEPENDS ${UNSTABLE_PROTOCOLS_DIR}/pointer-constraints/pointer-constraints-unstable-v1.xml
        COMMENT "Generating pointer-constraints protocol files"
    )

    add_custom_target(generate-protocol-files ALL
        DEPENDS 
        ${GENERATED_DIR}/relative-pointer-unstable-v1-client-protocol.h 
        ${GENERATED_DIR}/relative-pointer-unstable-v1-client-protocol.c
        ${GENERATED_DIR}/pointer-constraints-unstable-v1-client-protocol.h 
        ${GENERATED_DIR}/pointer-constraints-unstable-v1-client-protocol.c
        ${GENERATED_DIR}/xdg-shell-client-protocol.h
        ${GENERATED_DIR}/xdg-shell-protocol.c
    )

    target_include_directories(Wayland INTERFACE ${GENERATED_DIR})
    add_dependencies(Wayland generate-protocol-files)
endif()
    
#=======================================================================
# Direct3D12 Runtime
#=======================================================================
if(WIN32)
    add_subdirectory(./DirectX12)
endif()

#=======================================================================
# Shader Compilers
#=======================================================================
add_library(tools_shader_compilers INTERFACE)
target_link_libraries(tools_shader_compilers INTERFACE dxc)

if(WIN32 OR APPLE)
    target_link_libraries(tools_shader_compilers INTERFACE metal-shader-converter)
endif()

#=======================================================================
# Mesh Optimizer
#=======================================================================
if(NOT BUILD_FOR_IOS)
    add_subdirectory(./meshoptimizer)
endif()

#=======================================================================
# Fast Obj
#=======================================================================
if(NOT BUILD_FOR_IOS)
    add_subdirectory(./fast_obj)
endif()

#=======================================================================
# cgltf
#=======================================================================
if(NOT BUILD_FOR_IOS)
    add_library(cgltf INTERFACE)
    target_include_directories(cgltf INTERFACE ./cgltf/)
endif()

#=======================================================================
# stb
#=======================================================================
if(NOT BUILD_FOR_IOS)
    add_library(stb INTERFACE)
    target_include_directories(stb INTERFACE ./stb/)
endif()

#=======================================================================
# MikkTSpace
#=======================================================================
if(NOT BUILD_FOR_IOS)
    add_library(mikktspace INTERFACE)
    target_include_directories(mikktspace INTERFACE ./MikkTSpace/)
endif()

#=======================================================================
# bc7encoder_rdo
#=======================================================================
if(NOT BUILD_FOR_IOS)
    set(SUPPORT_BC7E FALSE) # TODO: Needs to set it to true for GPU accell and ISPC
    if(WIN32)
        set(MSVC FALSE)
    endif()
    #add_subdirectory(./bc7enc_rdo)
    add_library(bc7enc INTERFACE)
    target_include_directories(bc7enc INTERFACE ./bc7enc_rdo/)
endif()

#=======================================================================
# Tools External Dependencies
#=======================================================================
add_library(tools_external_dependencies INTERFACE)
target_link_libraries(tools_external_dependencies INTERFACE tools_shader_compilers meshoptimizer fast_obj cgltf stb mikktspace bc7enc)

#=======================================================================
# xxHash
#=======================================================================
add_library(xxHash INTERFACE)
target_include_directories(xxHash INTERFACE ./xxHash/)

#=======================================================================
# Volk
#=======================================================================
add_library(volk INTERFACE)
target_include_directories(volk INTERFACE ./volk/)

#=======================================================================
# Vulkan Headers
#=======================================================================
add_library(vulkan_headers INTERFACE)
target_include_directories(vulkan_headers INTERFACE ./Vulkan-Headers/include/)

#=======================================================================
# Metal-Cpp Headers
#=======================================================================
if(APPLE)
    add_library(metal_cpp INTERFACE)
    target_include_directories(metal_cpp INTERFACE ./metal-cpp/)
endif()

#=======================================================================
# External Dependencies
#=======================================================================
add_library(external_dependencies INTERFACE)
target_link_libraries(external_dependencies INTERFACE xxHash volk vulkan_headers)

if(APPLE)
    target_link_libraries(external_dependencies INTERFACE metal_cpp)
endif()

#=======================================================================
# UTest
#=======================================================================
add_library(utest INTERFACE)
target_include_directories(utest INTERFACE ./utest/)
