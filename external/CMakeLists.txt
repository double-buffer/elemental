#=======================================================================
# DirectX Shader Compiler
#=======================================================================
set(DXC_VERSION_OFFICIAL "v1.8.2403")
set(DXC_VERSION_CUSTOM "v2024-03-12")

add_library(dxc INTERFACE)

if(WIN32)
    get_github_release("microsoft/DirectXShaderCompiler" ${DXC_VERSION_OFFICIAL} "dxc_*.zip" "${CMAKE_CURRENT_BINARY_DIR}/shader-compilers/dxc/")
    target_include_directories(dxc INTERFACE ${CMAKE_CURRENT_BINARY_DIR}/shader-compilers/dxc/inc/dxc/)

    function(copy_dxc_to_target target)
        file(GLOB DXC_DLLS 
             "${CMAKE_BINARY_DIR}/external/shader-compilers/dxc/bin/x64/*.dll")
        foreach(DLL ${DXC_DLLS})
            get_filename_component(DLL_NAME "${DLL}" NAME)
            add_custom_command(TARGET ${target} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${DLL}"
                    "$<TARGET_FILE_DIR:${target}>/${DLL_NAME}")
        endforeach()
    endfunction()
elseif(LINUX)
    get_github_release("microsoft/DirectXShaderCompiler" ${DXC_VERSION_OFFICIAL} "linux_dxc_*.x86_64.tar.gz" "${CMAKE_CURRENT_BINARY_DIR}/shader-compilers/dxc/")
    target_include_directories(dxc INTERFACE ${CMAKE_CURRENT_BINARY_DIR}/shader-compilers/dxc/include/dxc/)
    target_include_directories(dxc INTERFACE ./DirectX-Headers/include/directx/)

    function(copy_dxc_to_target target)
        file(GLOB DXC_DLLS 
             "${CMAKE_BINARY_DIR}/external/shader-compilers/dxc/lib/x64/*.so")
        foreach(DLL ${DXC_DLLS})
            get_filename_component(DLL_NAME "${DLL}" NAME)
            add_custom_command(TARGET ${target} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${DLL}"
                    "$<TARGET_FILE_DIR:${target}>/${DLL_NAME}")
        endforeach()
    endfunction()
elseif(NOT BUILD_FOR_IOS)
    get_github_release("double-buffer/shader-compilers-bin" ${DXC_VERSION_CUSTOM} "macos_dxc_*_arm64.zip" "${CMAKE_CURRENT_BINARY_DIR}/shader-compilers/dxc/")
    target_include_directories(dxc INTERFACE ${CMAKE_CURRENT_BINARY_DIR}/shader-compilers/dxc/include/dxc/)
    target_include_directories(dxc INTERFACE ./DirectX-Headers/include/directx/)

    function(copy_dxc_to_target target)
        file(GLOB DXC_DLLS 
             "${CMAKE_BINARY_DIR}/external/shader-compilers/dxc/lib/*.dylib")
        foreach(DLL ${DXC_DLLS})
            get_filename_component(DLL_NAME "${DLL}" NAME)
            add_custom_command(TARGET ${target} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${DLL}"
                    "$<TARGET_FILE_DIR:${target}>/${DLL_NAME}")
        endforeach()
    endfunction()
endif()

#=======================================================================
# Metal Shader Converter
#=======================================================================
if (WIN32 OR APPLE)
    set(METAL_SHADER_CONVERTER_VERSION "v2024-03-12")

    add_library(metal-shader-converter INTERFACE)
    target_include_directories(metal-shader-converter INTERFACE ${CMAKE_CURRENT_BINARY_DIR}/shader-compilers/MetalShaderConverter/include/)
    target_link_directories(metal-shader-converter INTERFACE ${CMAKE_CURRENT_BINARY_DIR}/shader-compilers/MetalShaderConverter/lib/)
    target_link_libraries(metal-shader-converter INTERFACE metalirconverter)

    if(WIN32)
        get_github_release("double-buffer/shader-compilers-bin" ${METAL_SHADER_CONVERTER_VERSION} "windows_MetalShaderConverter_*_x64.zip" "${CMAKE_CURRENT_BINARY_DIR}/shader-compilers/MetalShaderConverter/")
        
        function(copy_MetalShaderConverter_to_target target)
            file(GLOB DXC_DLLS 
                 "${CMAKE_BINARY_DIR}/external/shader-compilers/MetalShaderConverter/lib/*.dll")
            foreach(DLL ${DXC_DLLS})
                get_filename_component(DLL_NAME "${DLL}" NAME)
                add_custom_command(TARGET ${target} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${DLL}"
                        "$<TARGET_FILE_DIR:${target}>/${DLL_NAME}")
            endforeach()
        endfunction()
    elseif(NOT BUILD_FOR_IOS)
        get_github_release("double-buffer/shader-compilers-bin" ${METAL_SHADER_CONVERTER_VERSION} "macos_MetalShaderConverter_*_arm64.zip" "${CMAKE_CURRENT_BINARY_DIR}/shader-compilers/MetalShaderConverter/")
        
        function(copy_MetalShaderConverter_to_target target)
            file(GLOB DXC_DLLS 
                 "${CMAKE_BINARY_DIR}/external/shader-compilers/MetalShaderConverter/lib/*.dylib")
            foreach(DLL ${DXC_DLLS})
                get_filename_component(DLL_NAME "${DLL}" NAME)
                add_custom_command(TARGET ${target} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${DLL}"
                        "$<TARGET_FILE_DIR:${target}>/${DLL_NAME}")
            endforeach()
        endfunction()
    endif()
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
elseif(NOT BUILD_FOR_IOS)
    get_github_release("double-buffer/shader-compilers-bin" ${SPIRV_CROSS_VERSION} "macos_spirv-cross_*_arm64.zip" "${CMAKE_CURRENT_BINARY_DIR}/shader-compilers/spirv-cross/")
    target_link_directories(spirv-cross INTERFACE ${CMAKE_CURRENT_BINARY_DIR}/shader-compilers/spirv-cross/lib/)
endif()
    
#=======================================================================
# Direct3D12 Runtime
#=======================================================================

if(WIN32)
    set(DIRECT3D12_VERSION "1.613.1")

    add_library(Direct3D12 INTERFACE)
    
    download_and_extract_nuget_package(Direct3D12 "Microsoft.Direct3D.D3D12" ${DIRECT3D12_VERSION})
    target_link_libraries(Direct3D12 INTERFACE dxgi dxguid)

    # TODO: Do we really need that?
    target_link_libraries(Direct3D12 INTERFACE d3d12)

    function(copy_direct3d12_to_target target)
        # TODO: To Fix
        set(DIRECT3D12_VERSION "1.613.1")
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

if(WIN32 OR APPLE)
    target_link_libraries(tools_shader_compilers INTERFACE metal-shader-converter)
endif()

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
