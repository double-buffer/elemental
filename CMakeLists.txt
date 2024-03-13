cmake_minimum_required(VERSION 3.26)

if(WIN32)
    set(CMAKE_SYSTEM_PROCESSOR "x64")
    set(CMAKE_CXX_COMPILER "clang-cl")
    set(CMAKE_C_COMPILER "clang-cl")
    
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

add_compile_options(-Wno-unused-parameter)
add_compile_options(-Wno-missing-field-initializers)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcolor-diagnostics")
set(CMAKE_CXX_STANDARD "23")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG")
set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -fcolor-diagnostics")
set(CMAKE_C_STANDARD "99")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_NAME_DIR "@rpath")
set(CMAKE_INSTALL_RPATH "@executable_path")

project(Elemental LANGUAGES C CXX)

# TODO: Use CMAKE INSTALL ?

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/utilities/cmake)
include(functions)

# TODO: Don't include tools dep for the runtime
add_subdirectory(external)
add_subdirectory(src/Elemental/Common)
add_subdirectory(src/Elemental)
add_subdirectory(src/Elemental.Tools)

add_subdirectory(tests/SystemFunctionsTests)
add_subdirectory(tests/ApplicationTests)

# TODO: Use a sub cmakefile?
add_subdirectory(samples/01-HelloWorld)
add_subdirectory(samples/02-HelloWindow)
add_subdirectory(samples/03-HelloTriangle)
