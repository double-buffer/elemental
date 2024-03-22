target_compile_options(Elemental.Tools PRIVATE -EHs)

target_include_directories(Elemental.Tools PRIVATE ../../Elemental/Microsoft)
target_link_libraries(Elemental.Tools PRIVATE Dwmapi UxTheme onecore)
target_compile_definitions(Elemental.Tools PRIVATE UNICODE _UNICODE _WINDOWS _WIN32)

function(copy_platform_dependencies target)
    copy_dxc_to_target(${target})
endfunction()
