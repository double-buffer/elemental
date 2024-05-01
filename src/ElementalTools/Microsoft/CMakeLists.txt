target_compile_options(ElementalTools PRIVATE -EHs)

target_include_directories(ElementalTools PRIVATE ../../Elemental/Microsoft)
target_link_libraries(ElementalTools PRIVATE Dwmapi UxTheme onecore)
target_compile_definitions(ElementalTools PRIVATE UNICODE _UNICODE _WINDOWS _WIN32)

function(copy_platform_dependencies target)
    copy_dxc_to_target(${target})
    copy_MetalShaderConverter_to_target(${target})
endfunction()
