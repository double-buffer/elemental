target_compile_options(Elemental PRIVATE -fno-objc-arc)
target_compile_definitions(Elemental PRIVATE __BLOCKS__)

if(BUILD_FOR_IOS)
    target_link_options(Elemental PRIVATE "SHELL:-framework Metal" "SHELL:-framework Foundation" "SHELL:-framework UIKit" "SHELL:-framework CoreGraphics" "SHELL:-framework IOKit")
else()
    target_link_options(Elemental PRIVATE "SHELL:-framework Metal" "SHELL:-framework MetalKit" "SHELL:-framework Foundation" "SHELL:-framework Cocoa" "SHELL:-framework CoreGraphics" "SHELL:-framework IOKit")
endif()

function(copy_platform_dependencies target)
endfunction()
