cmake_minimum_required(VERSION 3.26)
project(Elemental LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)
include(functions)

add_subdirectory(external)
add_subdirectory(src/Platforms/Common)
add_subdirectory(src/Elemental.Tools.Native)