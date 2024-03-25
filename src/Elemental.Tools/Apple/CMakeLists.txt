target_include_directories(Elemental.Tools PRIVATE ../../Elemental/Apple/)
target_include_directories(Elemental.Tools PRIVATE ./Interop/)
target_compile_options(Elemental.Tools PRIVATE -fms-extensions -fno-objc-arc)
target_link_options(Elemental.Tools PRIVATE "SHELL:-framework Foundation")

target_compile_options(Elemental PRIVATE -fno-objc-arc)
target_compile_definitions(Elemental PRIVATE __BLOCKS__)
target_link_options(Elemental PRIVATE "SHELL:-framework Metal" "SHELL:-framework Foundation" "SHELL:-framework Cocoa" "SHELL:-framework CoreGraphics" "SHELL:-framework IOKit")

function(copy_platform_dependencies target)
    copy_dxc_to_target(${target})
endfunction()
