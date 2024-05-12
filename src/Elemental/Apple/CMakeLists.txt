target_compile_options(Elemental PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-fno-objc-arc>)
target_compile_definitions(Elemental PRIVATE $<$<COMPILE_LANGUAGE:CXX>:__BLOCKS__>)

target_compile_options(Elemental PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wno-nullability-extension>)
target_compile_options(Elemental PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wno-c11-extensions>)

if(BUILD_FOR_IOS)
    target_link_options(Elemental PRIVATE "SHELL:-framework Metal" "SHELL:-framework QuartzCore" "SHELL:-framework Foundation" "SHELL:-framework UIKit" "SHELL:-framework CoreGraphics" "SHELL:-framework IOKit")
else()
    target_link_options(Elemental PRIVATE "SHELL:-framework Metal" "SHELL:-framework QuartzCore" "SHELL:-framework Foundation" "SHELL:-framework Cocoa" "SHELL:-framework CoreGraphics" "SHELL:-framework IOKit")
endif()

set(ELEMENTAL_OUTPUT_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Elemental.framework" PARENT_SCOPE)
        
#et(HEADERS "../Elemental.h" "../ElementalLoader.c")
#set_source_files_properties(ElementalLoader.c PROPERTIES HEADER_FILE_ONLY TRUE)
#target_sources(Elemental PUBLIC ${HEADERS})

if(BUILD_FOR_IOS)
set_target_properties(Elemental PROPERTIES
  FRAMEWORK TRUE
  FRAMEWORK_VERSION A
  MACOSX_FRAMEWORK_IDENTIFIER Elemental
  MACOSX_FRAMEWORK_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist
  XCODE_ATTRIBUTE_DEVELOPMENT_TEAM ${APPLE_SIGNING_TEAM_ID}
  XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "Apple Development"
  VERSION 1.0.0
  SOVERSION 1.0.0
  PUBLIC_HEADER "${HEADERS}"
)
else()
set_target_properties(Elemental PROPERTIES
  FRAMEWORK TRUE
  FRAMEWORK_VERSION A
  MACOSX_FRAMEWORK_IDENTIFIER Elemental
  MACOSX_FRAMEWORK_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist
  VERSION 1.0.0
  SOVERSION 1.0.0
  PUBLIC_HEADER "${HEADERS}"
)
endif()

function(copy_platform_dependencies target)
endfunction()


set(CMAKE_Swift_LANGUAGE_VERSION 5)
set(HEADER_OUTPUT_PATH "${CMAKE_BINARY_DIR}/ElementalSwiftLib.h")
add_library(ElementalSwiftLib STATIC Graphics/MetalView.swift)
 
target_compile_options(ElementalSwiftLib PRIVATE -cxx-interoperability-mode=default)
target_compile_options(ElementalSwiftLib PRIVATE -emit-clang-header-path "${HEADER_OUTPUT_PATH}")
target_include_directories(ElementalSwiftLib INTERFACE "${CMAKE_BINARY_DIR}/")
set_target_properties(ElementalSwiftLib PROPERTIES XCODE_ATTRIBUTE_SWIFT_OBJC_INTERFACE_HEADER_NAME "")

target_link_libraries(Elemental PRIVATE ElementalSwiftLib)

add_custom_command(
    OUTPUT ${HEADER_OUTPUT_PATH}
    COMMAND echo "Waiting for ${HEADER_OUTPUT_PATH} to be generated."
    DEPENDS ElementalSwiftLib
    COMMENT "Checking for existence of ElementalSwiftLib.h"
)

add_custom_target(
    CheckForSwiftHeader ALL
    DEPENDS ${HEADER_OUTPUT_PATH}
)

add_dependencies(Elemental CheckForSwiftHeader)
    
# TODO: For Release Support macOS xc framework!

