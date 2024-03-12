#=======================================================================
# DirectX Shader Compiler
#=======================================================================
set(DXC_VERSION_OFFICIAL "v1.8.2403")
set(DXC_VERSION_CUSTOM "v2024-03-12")

add_library(dxc INTERFACE)

# TODO: Get rid of macos x64
if(WIN32)
    get_github_release("microsoft/DirectXShaderCompiler" ${DXC_VERSION_OFFICIAL} "dxc_*.zip" "${CMAKE_CURRENT_BINARY_DIR}/shader-compilers/dxc/")
    target_include_directories(dxc INTERFACE ${CMAKE_CURRENT_BINARY_DIR}/shader-compilers/dxc/inc/dxc/)
elseif(APPLE)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64")
        message(STATUS "Building for ARM64")
        get_github_release("double-buffer/shader-compilers-bin" ${DXC_VERSION_CUSTOM} "macos_dxc_*_arm64.zip" "${CMAKE_CURRENT_BINARY_DIR}/shader-compilers/dxc/")
    else()
        message(STATUS "Not building for ARM64")
        get_github_release("double-buffer/shader-compilers-bin" ${DXC_VERSION_CUSTOM} "macos_dxc_*_x64.zip" "${CMAKE_CURRENT_BINARY_DIR}/shader-compilers/dxc/")
    endif()
    target_include_directories(dxc INTERFACE ${CMAKE_CURRENT_BINARY_DIR}/shader-compilers/dxc/include/dxc/)
    target_include_directories(dxc INTERFACE ./DirectX-Headers/include/directx/)
endif()

#=======================================================================
# SPIRV-Cross Compiler
#=======================================================================
set(SPIRV_CROSS_VERSION "v2024-03-12")

add_library(spirv-cross INTERFACE)
target_include_directories(spirv-cross INTERFACE ${CMAKE_CURRENT_BINARY_DIR}/shader-compilers/spirv-cross/include/)
target_link_libraries(spirv-cross INTERFACE spirv-cross-core spirv-cross-cpp spirv-cross-msl spirv-cross-glsl spirv-cross-reflect spirv-cross-util)

if(WIN32)
    get_github_release("double-buffer/shader-compilers-bin" ${SPIRV_CROSS_VERSION} "windows_spirv-cross_*_x64.zip" "${CMAKE_CURRENT_BINARY_DIR}/shader-compilers/spirv-cross/")
    target_link_directories(spirv-cross INTERFACE ${CMAKE_CURRENT_BINARY_DIR}/shader-compilers/spirv-cross/lib/lib/)
elseif(APPLE)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64")
        message(STATUS "Building for ARM64")
        get_github_release("double-buffer/shader-compilers-bin" ${SPIRV_CROSS_VERSION} "macos_spirv-cross_*_arm64.zip" "${CMAKE_CURRENT_BINARY_DIR}/shader-compilers/spirv-cross/")
    else()
        message(STATUS "Not building for ARM64")
        get_github_release("double-buffer/shader-compilers-bin" ${SPIRV_CROSS_VERSION} "macos_spirv-cross_*_x64.zip" "${CMAKE_CURRENT_BINARY_DIR}/shader-compilers/spirv-cross/")
    endif()

    target_link_directories(spirv-cross INTERFACE ${CMAKE_CURRENT_BINARY_DIR}/shader-compilers/spirv-cross/lib/)
endif()
    
#=======================================================================
# Direct3D12 Runtime
#=======================================================================

if(WIN32)
    set(DIRECT3D12_VERSION "1.611.2")

    add_library(Direct3D12 INTERFACE)
    
    download_and_extract_nuget_package(Direct3D12 "Microsoft.Direct3D.D3D12" ${DIRECT3D12_VERSION})
    target_link_libraries(Direct3D12 INTERFACE dxgi dxguid)

    # TODO: Do we really need that?
    target_link_libraries(Direct3D12 INTERFACE d3d12)

    function(copy_direct3d12_to_target target)
        # TODO: To Fix
        set(DIRECT3D12_VERSION "1.611.2")
        file(GLOB DIRECT3D12_DLLS 
             "${CMAKE_BINARY_DIR}/external/packages/Microsoft.Direct3D.D3D12_${DIRECT3D12_VERSION}/build/native/bin/x64/*.dll")
        foreach(DLL ${DIRECT3D12_DLLS})
            get_filename_component(DLL_NAME "${DLL}" NAME)
            add_custom_command(TARGET ${target} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${DLL}"
                    "$<TARGET_FILE_DIR:${target}>/${DLL_NAME}")
        endforeach()
    endfunction()
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
