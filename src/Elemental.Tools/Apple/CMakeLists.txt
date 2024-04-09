target_include_directories(Elemental.Tools PRIVATE ../../Elemental/Apple/)
target_include_directories(Elemental.Tools PRIVATE ./Interop/)
target_compile_options(Elemental.Tools PRIVATE -fms-extensions -fno-objc-arc -Wno-language-extension-token)
target_link_options(Elemental.Tools PRIVATE "SHELL:-framework Foundation")

set_target_properties(Elemental PROPERTIES
  FRAMEWORK TRUE
  FRAMEWORK_VERSION A
  MACOSX_FRAMEWORK_IDENTIFIER ElementalTools
  MACOSX_FRAMEWORK_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist
  VERSION 16.4.0
  SOVERSION 1.0.0
  PUBLIC_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/../ElementalTools.h
)

target_compile_options(Elemental PRIVATE -fno-objc-arc)
target_compile_definitions(Elemental PRIVATE __BLOCKS__)
target_link_options(Elemental PRIVATE "SHELL:-framework Metal" "SHELL:-framework Foundation" "SHELL:-framework Cocoa" "SHELL:-framework CoreGraphics" "SHELL:-framework IOKit")

function(copy_platform_dependencies target)
    copy_dxc_to_target(${target})
    copy_MetalShaderConverter_to_target(${target})
endfunction()
