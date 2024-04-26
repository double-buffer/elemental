target_include_directories(Elemental.Tools PRIVATE ../../Elemental/Linux/)
target_include_directories(Elemental.Tools PRIVATE ./Interop/)
target_compile_options(Elemental.Tools PRIVATE -fms-extensions -Wno-language-extension-token)

function(copy_platform_dependencies target)
    copy_dxc_to_target(${target})
endfunction()
