cmake_minimum_required(VERSION 3.26)

if(WIN32)
    set(CMAKE_SYSTEM_PROCESSOR "x64")
    set(CMAKE_CXX_COMPILER "clang-cl")
endif()
    
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcolor-diagnostics")
set(CMAKE_CXX_STANDARD "23")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(Elemental LANGUAGES CXX)

# Set Global properties like C++ Version etc.

# TODO: Find a way to have only one build directory

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)
include(functions)

# TODO: Don't include tools dep for the runtime
add_subdirectory(external)
add_subdirectory(src/Elemental.Native/Common)
add_subdirectory(src/Elemental.Tools.Native)
add_subdirectory(tests/Elemental.Tests.Native.SystemFunctionsTests)

if(WIN32)
    add_subdirectory(src/Elemental.Native/Windows)
elseif(APPLE)
    add_subdirectory(src/Elemental.Native/Apple)
endif()
