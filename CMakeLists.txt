cmake_minimum_required(VERSION 3.26)
project(Elemental LANGUAGES CXX)

# Set Global properties like C++ Version etc.

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)
include(functions)

add_subdirectory(external)
add_subdirectory(src/Platforms/Common)
add_subdirectory(src/Elemental.Tools.Native)

if(WIN32)
    add_subdirectory(src/Platforms/Windows)
endif()