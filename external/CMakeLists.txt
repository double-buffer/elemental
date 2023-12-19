#=======================================================================
# DirectX Shader Compiler
#=======================================================================
add_library(dxc INTERFACE)

if(WIN32)
    get_github_release("microsoft/DirectXShaderCompiler" "v1.7.2308" "dxc_*.zip" "${CMAKE_CURRENT_BINARY_DIR}/shader-compilers/dxc/")
    target_include_directories(dxc INTERFACE ${CMAKE_CURRENT_BINARY_DIR}/shader-compilers/dxc/inc/dxc/)
elseif(APPLE)
    get_github_release("double-buffer/shader-compilers-bin" "v2023-11-25" "macos_dxc_*.zip" "${CMAKE_CURRENT_BINARY_DIR}/shader-compilers/dxc/")
    target_include_directories(dxc INTERFACE ${CMAKE_CURRENT_BINARY_DIR}/shader-compilers/dxc/include/dxc/)
    target_include_directories(dxc INTERFACE ./DirectX-Headers/include/directx/)
endif()

#=======================================================================
# SPIRV-Cross Compiler
#=======================================================================
add_library(spirv-cross INTERFACE)
target_include_directories(spirv-cross INTERFACE ${CMAKE_CURRENT_BINARY_DIR}/shader-compilers/spirv-cross/include/)
target_link_libraries(spirv-cross INTERFACE spirv-cross-core spirv-cross-cpp spirv-cross-msl spirv-cross-glsl spirv-cross-reflect spirv-cross-util)

if(WIN32)
    get_github_release("double-buffer/shader-compilers-bin" "v2023-11-25" "windows_spirv-cross_*_x64.zip" "${CMAKE_CURRENT_BINARY_DIR}/shader-compilers/spirv-cross/")
    target_link_directories(spirv-cross INTERFACE ${CMAKE_CURRENT_BINARY_DIR}/shader-compilers/spirv-cross/lib/lib/)
elseif(APPLE)
    get_github_release("double-buffer/shader-compilers-bin" "v2023-11-25" "macos_spirv-cross_*_x64.zip" "${CMAKE_CURRENT_BINARY_DIR}/shader-compilers/spirv-cross/")
    target_link_directories(spirv-cross INTERFACE ${CMAKE_CURRENT_BINARY_DIR}/shader-compilers/spirv-cross/lib/)
endif()
    
#=======================================================================
# Shader Compilers
#=======================================================================
add_library(tools_shader_compilers INTERFACE)
target_link_libraries(tools_shader_compilers INTERFACE spirv-cross dxc)

#=======================================================================
# Tools External Dependencies
#=======================================================================
add_library(tools_external_dependencies INTERFACE)
target_link_libraries(tools_external_dependencies INTERFACE tools_shader_compilers)

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
