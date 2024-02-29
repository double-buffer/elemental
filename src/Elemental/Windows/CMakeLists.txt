add_library(Direct3D12 INTERFACE)
download_and_extract_nuget_package(Direct3D12 "Microsoft.Direct3D.D3D12" "1.611.2")
target_link_libraries(Direct3D12 INTERFACE dxgi dxguid)

# TODO: Do we really need that?
target_link_libraries(Direct3D12 INTERFACE d3d12)

function(copy_direct3d12_to_target target)
    file(GLOB DIRECT3D12_DLLS 
         "${CMAKE_CURRENT_BINARY_DIR}/Windows/packages/Microsoft.Direct3D.D3D12_1.611.2/build/native/bin/x64/*.dll")
    
    foreach(DLL ${DIRECT3D12_DLLS})
        get_filename_component(DLL_NAME "${DLL}" NAME)
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${DLL}"
                "$<TARGET_FILE_DIR:${target}>/${DLL_NAME}")
    endforeach()
endfunction()


target_compile_options(Elemental PRIVATE -EHs)

target_link_libraries(Elemental PRIVATE Direct3D12)

target_link_libraries(Elemental PRIVATE Dwmapi UxTheme onecore)
target_compile_definitions(Elemental PRIVATE UNICODE _UNICODE _WINDOWS _WIN32)

function(copy_platform_dependencies target)
    copy_direct3d12_to_target(${target})
endfunction()
