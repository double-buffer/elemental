cmake_minimum_required(VERSION 3.28)
include(ExternalProject)

set(CMAKE_CXX_COMPILER "clang")
set(CMAKE_C_COMPILER "clang")

if(WIN32)
    set(CMAKE_SYSTEM_PROCESSOR "x64")
    set(CMAKE_CXX_COMPILER "clang-cl")
    set(CMAKE_C_COMPILER "clang-cl")
    
    add_compile_options(/W4)
else()
    if(BUILD_FOR_IOS)
        set(CMAKE_SYSTEM_NAME iOS)
        set(CMAKE_OSX_SYSROOT iphoneos)
    endif()
    set(CLANG_COMPILE_OPTIONS -Wall -Wextra -Wpedantic)
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wall>)
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wextra>)
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wpedantic>)
    add_compile_options($<$<COMPILE_LANGUAGE:C>:-Wall>)
    add_compile_options($<$<COMPILE_LANGUAGE:C>:-Wextra>)
    add_compile_options($<$<COMPILE_LANGUAGE:C>:-Wpedantic>)
endif()

add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wno-unused-parameter>)
add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wno-missing-field-initializers>)
add_compile_options($<$<COMPILE_LANGUAGE:C>:-Wno-unused-parameter>)
add_compile_options($<$<COMPILE_LANGUAGE:C>:-Wno-missing-field-initializers>)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcolor-diagnostics")
set(CMAKE_CXX_STANDARD "23")
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(LINUX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
endif()

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -D_DEBUG")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fcolor-diagnostics")
set(CMAKE_C_STANDARD "99")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_NAME_DIR "@rpath")

if(APPLE)
    set(CMAKE_INSTALL_RPATH "@executable_path;@executable_path/../Frameworks;@executable_path/Frameworks;@loader_path")
elseif(LINUX)
    set(CMAKE_INSTALL_RPATH "$ORIGIN;$ORIGIN/../Frameworks;$ORIGIN/Frameworks")
endif()

if(APPLE)
    project(Elemental LANGUAGES C CXX Swift)
else()
    project(Elemental LANGUAGES C CXX)
endif()

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/utilities/cmake)
include(functions)

# TODO: Don't include tools dep for the runtime
add_subdirectory(external)
add_subdirectory(src/Elemental/Common)
add_subdirectory(src/Elemental)
    
if(NOT BUILD_FOR_IOS)
add_subdirectory(src/ElementalTools)
endif()

add_subdirectory(tests/SystemFunctionsTests)
add_subdirectory(tests/ApplicationTests)
add_subdirectory(tests/GraphicsTests)

if(NOT BUILD_FOR_IOS)
add_subdirectory(tests/ToolsTests)
endif()

add_subdirectory(samples)

# DEBUG Deps
#set_property(GLOBAL PROPERTY GLOBAL_DEPENDS_DEBUG_MODE 1)
