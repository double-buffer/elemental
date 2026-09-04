cmake_minimum_required(VERSION 3.28)

if(WIN32)
    set(CMAKE_SYSTEM_PROCESSOR "x64")
    set(CMAKE_CXX_COMPILER "clang-cl")
    set(CMAKE_C_COMPILER "clang-cl")

    add_compile_options(/W4)
elseif(LINUX)
    set(CMAKE_CXX_COMPILER "clang++")
    set(CMAKE_C_COMPILER "clang")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lm")
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
else()
    if(BUILD_FOR_IOS)
        set(CMAKE_SYSTEM_NAME iOS)
        set(CMAKE_OSX_SYSROOT iphoneos)

        if(NOT DEFINED APPLE_SIGNING_TEAM_ID)
            message(FATAL_ERROR "When building for iOS you need to pass signing information.")
        endif()
    endif()

    add_compile_options(
        $<$<COMPILE_LANGUAGE:C,CXX>:-Wall>
        $<$<COMPILE_LANGUAGE:C,CXX>:-Wextra>
        $<$<COMPILE_LANGUAGE:C,CXX>:-Wpedantic>
    )
endif()

add_compile_options(
    $<$<COMPILE_LANGUAGE:C,CXX>:-Wno-unused-parameter>
    $<$<COMPILE_LANGUAGE:C,CXX>:-Wno-missing-field-initializers>
    $<$<COMPILE_LANGUAGE:C,CXX>:-fansi-escape-codes>
    $<$<COMPILE_LANGUAGE:C,CXX>:-fcolor-diagnostics>
)

add_compile_definitions(
    _CRT_SECURE_NO_WARNINGS
    $<$<CONFIG:Debug>:_DEBUG>
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

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

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/ExternalPackages.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/ResourceCompilation.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/AppPackaging.cmake")

add_subdirectory(external)
add_subdirectory(src/Elemental/Common)
add_subdirectory(src/Elemental)

if(NOT BUILD_FOR_IOS)
    add_subdirectory(src/ElementalTools)
endif()

add_subdirectory(samples)

set(SUB_PROJECTS tests samples)
foreach(SUB_PROJECT IN LISTS SUB_PROJECTS)
    file(GLOB_RECURSE SUB_PROJECT_FILES "${SUB_PROJECT}/**/CMakeLists.txt")
    foreach(SUB_PROJECT_FILE IN LISTS SUB_PROJECT_FILES)
        get_filename_component(SUB_PROJECT_DIR ${SUB_PROJECT_FILE} DIRECTORY)
        add_subdirectory(${SUB_PROJECT_DIR})
    endforeach()
endforeach()
