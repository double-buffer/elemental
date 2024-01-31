add_library(Direct3D12 INTERFACE)
download_and_extract_nuget_package(Direct3D12 "Microsoft.Direct3D.D3D12" "1.611.2")
target_link_libraries(Direct3D12 INTERFACE dxgi dxguid)



# TODO: Do we really need that?
target_link_libraries(Direct3D12 INTERFACE d3d12)

add_library(Elemental SHARED UnityBuild.cpp)

target_compile_options(Elemental PRIVATE -W4 -EHs)
target_compile_options(Elemental PRIVATE -Wno-unused-parameter)
target_compile_options(Elemental PRIVATE -Wno-missing-field-initializers)
target_precompile_headers(Elemental PRIVATE PreCompiledHeader.h)

target_include_directories(Elemental PRIVATE .)
target_link_libraries(Elemental PRIVATE common)
target_link_libraries(Elemental PRIVATE external_dependencies)
target_link_libraries(Elemental PRIVATE Direct3D12)
target_link_libraries(Elemental PRIVATE Dwmapi UxTheme onecore)
target_compile_definitions(Elemental PRIVATE UNICODE _UNICODE _WINDOWS _WIN32)

add_custom_command(TARGET Elemental POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_BINARY_DIR}/packages/Microsoft.Direct3D.D3D12_1.611.2/build/native/bin/x64"
        $<TARGET_FILE_DIR:Elemental>)
