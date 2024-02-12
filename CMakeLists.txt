cmake_minimum_required(VERSION 3.26)

if(WIN32)
    set(CMAKE_SYSTEM_PROCESSOR "x64")
    set(CMAKE_CXX_COMPILER "clang-cl")
    set(CMAKE_C_COMPILER "clang-cl")
endif()
    
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcolor-diagnostics")
set(CMAKE_CXX_STANDARD "23")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG")
set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -fcolor-diagnostics")
set(CMAKE_C_STANDARD "99")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(Elemental LANGUAGES C CXX)

# TODO: Use CMAKE INSTALL !!!!!

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)
include(functions)

# TODO: Don't include tools dep for the runtime
add_subdirectory(external)
add_subdirectory(src/Elemental/Common)
add_subdirectory(src/Elemental.Tools)


if(WIN32)
    add_subdirectory(src/Elemental/Windows)
elseif(APPLE)
    add_subdirectory(src/Elemental/Apple)
endif()

add_subdirectory(tests/SystemFunctionsTests)

add_subdirectory(samples/01-HelloWorld)
