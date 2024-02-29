target_compile_options(Elemental PRIVATE -EHs)

target_link_libraries(Elemental PRIVATE Direct3D12)

target_link_libraries(Elemental PRIVATE Dwmapi UxTheme onecore)
target_compile_definitions(Elemental PRIVATE UNICODE _UNICODE _WINDOWS _WIN32)

function(copy_platform_dependencies target)
    copy_direct3d12_to_target(${target})
endfunction()
