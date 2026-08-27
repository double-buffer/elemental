if (WIN32 OR APPLE)
    set(METAL_SHADER_CONVERTER_VERSION "v2025-01-07")

    add_library(metal-shader-converter INTERFACE)
    target_include_directories(metal-shader-converter INTERFACE ${CMAKE_CURRENT_BINARY_DIR}/MetalShaderConverter/include/)
    target_link_directories(metal-shader-converter INTERFACE ${CMAKE_CURRENT_BINARY_DIR}/MetalShaderConverter/lib/)
    target_link_libraries(metal-shader-converter INTERFACE metalirconverter)

    if(WIN32)
        get_github_release("double-buffer/shader-compilers-bin" ${METAL_SHADER_CONVERTER_VERSION} "windows_MetalShaderConverter_*_x64.zip" "${CMAKE_CURRENT_BINARY_DIR}/MetalShaderConverter/")
        
        function(copy_MetalShaderConverter_to_target target)
            file(GLOB DXC_DLLS 
                 "${CMAKE_BINARY_DIR}/external/MetalShaderConverter/MetalShaderConverter/lib/*.dll")
            foreach(DLL ${DXC_DLLS})
                get_filename_component(DLL_NAME "${DLL}" NAME)
                add_custom_command(TARGET ${target} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${DLL}"
                        "$<TARGET_FILE_DIR:${target}>/${DLL_NAME}")
            endforeach()
        endfunction()
    elseif(NOT BUILD_FOR_IOS)
        get_github_release("double-buffer/shader-compilers-bin" ${METAL_SHADER_CONVERTER_VERSION} "macos_MetalShaderConverter_*_arm64.zip" "${CMAKE_CURRENT_BINARY_DIR}/MetalShaderConverter/")
        
        function(copy_MetalShaderConverter_to_target target)
            file(GLOB DXC_DLLS 
                 "${CMAKE_BINARY_DIR}/external/MetalShaderConverter/MetalShaderConverter/lib/*.dylib")
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
