cmake_minimum_required(VERSION 3.26)
project(Elemental LANGUAGES CXX)

if(MSVC)
    set(CMAKE_SYSTEM_PROCESSOR ${MSVC_CXX_ARCHITECTURE_ID})
endif()
    
if(WIN32)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG")
endif()

# Set Global properties like C++ Version etc.

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)
include(functions)

add_subdirectory(external)
add_subdirectory(src/Platforms/Common)
add_subdirectory(src/Elemental.Tools.Native)

if(WIN32)
    add_subdirectory(src/Platforms/Windows)
endif()