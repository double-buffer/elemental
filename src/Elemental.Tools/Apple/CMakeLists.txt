target_compile_options(Elemental PRIVATE -fno-objc-arc)
target_compile_definitions(Elemental PRIVATE __BLOCKS__)
target_link_options(Elemental PRIVATE "SHELL:-framework Metal" "SHELL:-framework Foundation" "SHELL:-framework Cocoa" "SHELL:-framework CoreGraphics" "SHELL:-framework IOKit")

function(copy_platform_dependencies target)
endfunction()
