add_library(Direct3D12 INTERFACE)
download_and_extract_nuget_package(Direct3D12 "Microsoft.Direct3D.D3D12" "1.610.2")
target_link_libraries(Direct3D12 INTERFACE dxgi dxguid)



# TODO: Do we really need that?
target_link_libraries(Direct3D12 INTERFACE d3d12)

add_library(Elemental.Native SHARED UnityBuild.cpp)

target_compile_options(Elemental.Native PRIVATE -W4 -EHs)
target_compile_options(Elemental.Native PRIVATE -Wno-unused-parameter)
target_compile_options(Elemental.Native PRIVATE -Wno-missing-field-initializers)
target_compile_features(Elemental.Native PRIVATE cxx_std_20)
target_precompile_headers(Elemental.Native PRIVATE PreCompiledHeader.h)

target_include_directories(Elemental.Native PRIVATE .)
target_link_libraries(Elemental.Native PRIVATE common)
target_link_libraries(Elemental.Native PRIVATE external_dependencies)
target_link_libraries(Elemental.Native PRIVATE Direct3D12)
target_link_libraries(Elemental.Native PRIVATE Dwmapi UxTheme)
target_compile_definitions(Elemental.Native PRIVATE UNICODE _UNICODE _WINDOWS _WIN32)

add_custom_command(TARGET Elemental.Native POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_BINARY_DIR}/packages/Microsoft.Direct3D.D3D12_1.610.2/build/native/bin/${CMAKE_SYSTEM_PROCESSOR}"
        $<TARGET_FILE_DIR:Elemental.Native>)
