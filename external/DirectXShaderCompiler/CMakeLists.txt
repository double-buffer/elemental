set(DXC_VERSION_CUSTOM "v2025-01-07")

add_library(dxc INTERFACE)

if(WIN32)
    get_github_release("double-buffer/shader-compilers-bin" ${DXC_VERSION_CUSTOM} "windows_dxc_*_x64.zip" "${CMAKE_CURRENT_BINARY_DIR}/dxc/")
    target_include_directories(dxc INTERFACE ${CMAKE_CURRENT_BINARY_DIR}/dxc/include/dxc/)

    function(copy_dxc_to_target target)
        file(GLOB DXC_DLLS 
             "${CMAKE_BINARY_DIR}/external/DirectXShaderCompiler/dxc/lib/*.dll")
        foreach(DLL ${DXC_DLLS})
            get_filename_component(DLL_NAME "${DLL}" NAME)
            add_custom_command(TARGET ${target} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${DLL}"
                    "$<TARGET_FILE_DIR:${target}>/${DLL_NAME}")
        endforeach()
    endfunction()
elseif(LINUX)
    get_github_release("double-buffer/shader-compilers-bin" ${DXC_VERSION_CUSTOM} "linux_dxc_*_x64.zip" "${CMAKE_CURRENT_BINARY_DIR}/dxc/")
    target_include_directories(dxc INTERFACE ${CMAKE_CURRENT_BINARY_DIR}/dxc/include/dxc/)
    target_include_directories(dxc INTERFACE ../DirectX-Headers/include/directx/)

    function(copy_dxc_to_target target)
        file(GLOB DXC_DLLS 
             "${CMAKE_BINARY_DIR}/external/DirectXShaderCompiler/dxc/lib/*.so")
        foreach(DLL ${DXC_DLLS})
            get_filename_component(DLL_NAME "${DLL}" NAME)
            add_custom_command(TARGET ${target} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${DLL}"
                    "$<TARGET_FILE_DIR:${target}>/${DLL_NAME}")
        endforeach()
    endfunction()
elseif(NOT BUILD_FOR_IOS)
    get_github_release("double-buffer/shader-compilers-bin" ${DXC_VERSION_CUSTOM} "macos_dxc_*_arm64.zip" "${CMAKE_CURRENT_BINARY_DIR}/dxc/")
    target_include_directories(dxc INTERFACE ${CMAKE_CURRENT_BINARY_DIR}/dxc/include/dxc/)
    target_include_directories(dxc INTERFACE ../DirectX-Headers/include/directx/)

    function(copy_dxc_to_target target)
        file(GLOB DXC_DLLS 
             "${CMAKE_BINARY_DIR}/external/DirectXShaderCompiler/dxc/lib/*.dylib")
        foreach(DLL ${DXC_DLLS})
            get_filename_component(DLL_NAME "${DLL}" NAME)
            add_custom_command(TARGET ${target} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${DLL}"
                    "$<TARGET_FILE_DIR:${target}>/${DLL_NAME}")
        endforeach()
    endfunction()
endif()
