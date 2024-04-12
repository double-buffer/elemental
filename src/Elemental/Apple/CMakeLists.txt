target_compile_options(Elemental PRIVATE -fno-objc-arc)
target_compile_definitions(Elemental PRIVATE __BLOCKS__)

if(BUILD_FOR_IOS)
    target_link_options(Elemental PRIVATE "SHELL:-framework MetalKit" "SHELL:-framework Metal" "SHELL:-framework Foundation" "SHELL:-framework UIKit" "SHELL:-framework CoreGraphics" "SHELL:-framework IOKit")
else()
    target_link_options(Elemental PRIVATE "SHELL:-framework MetalKit" "SHELL:-framework Metal" "SHELL:-framework Foundation" "SHELL:-framework Cocoa" "SHELL:-framework CoreGraphics" "SHELL:-framework IOKit")
endif()
        
set(HEADERS "../Elemental.h" "../ElementalLoader.c")
target_sources(Elemental PUBLIC ${HEADERS})

set_target_properties(Elemental PROPERTIES
  FRAMEWORK TRUE
  FRAMEWORK_VERSION A
  MACOSX_FRAMEWORK_IDENTIFIER Elemental
  MACOSX_FRAMEWORK_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist
  VERSION 1.0.0
  SOVERSION 1.0.0
  PUBLIC_HEADER "${HEADERS}"
)

function(copy_platform_dependencies target)
endfunction()

