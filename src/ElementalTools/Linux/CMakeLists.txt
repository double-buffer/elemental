target_include_directories(ElementalTools PRIVATE ../../Elemental/Linux/)
target_include_directories(ElementalTools PRIVATE ./Interop/)
target_compile_options(ElementalTools PRIVATE -fms-extensions -Wno-language-extension-token)

function(copy_platform_dependencies target)
    copy_dxc_to_target(${target})
endfunction()
