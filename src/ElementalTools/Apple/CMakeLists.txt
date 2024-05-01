target_include_directories(ElementalTools PRIVATE ../../Elemental/Apple/)
target_include_directories(ElementalTools PRIVATE ./Interop/)
target_compile_options(ElementalTools PRIVATE -fms-extensions -fno-objc-arc -Wno-language-extension-token)
target_link_options(ElementalTools PRIVATE "SHELL:-framework Foundation")

set_target_properties(ElementalTools PROPERTIES
  FRAMEWORK TRUE
  FRAMEWORK_VERSION A
  MACOSX_FRAMEWORK_IDENTIFIER ElementalTools
  MACOSX_FRAMEWORK_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist
  VERSION 1.0.0
  SOVERSION 1.0.0
  PUBLIC_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/../ElementalTools.h
)

function(copy_platform_dependencies target)
    copy_dxc_to_target(${target})
    copy_MetalShaderConverter_to_target(${target})
endfunction()
