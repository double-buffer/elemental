cmake_minimum_required(VERSION 3.28)

if(WIN32)
    set(CMAKE_SYSTEM_PROCESSOR "x64")
    set(CMAKE_CXX_COMPILER "clang-cl")
    set(CMAKE_C_COMPILER "clang-cl")
    
    add_compile_options(/W4)
elseif(LINUX)
    set(CMAKE_CXX_COMPILER "clang++")
    set(CMAKE_C_COMPILER "clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lm")
else()
    if(BUILD_FOR_IOS)
        set(CMAKE_SYSTEM_NAME iOS)
        set(CMAKE_OSX_SYSROOT iphoneos)

        if(NOT DEFINED APPLE_SIGNING_TEAM_ID)
            message(FATAL_ERROR "When building for iOS you need to pass signing information.")
        endif()
    endif()
    set(CLANG_COMPILE_OPTIONS -Wall -Wextra -Wpedantic)
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wall>)
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wextra>)
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wpedantic>)
    add_compile_options($<$<COMPILE_LANGUAGE:C>:-Wall>)
    add_compile_options($<$<COMPILE_LANGUAGE:C>:-Wextra>)
    add_compile_options($<$<COMPILE_LANGUAGE:C>:-Wpedantic>)

    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g")

    find_program(DSYMUTIL_PROGRAM dsymutil)
    # TODO: This method work when we need to debug but it causes errors when recompiling
    #[[
    if (DSYMUTIL_PROGRAM)
      foreach(lang C CXX)
        foreach(var LINK_EXECUTABLE CREATE_SHARED_LIBRARY)
          set(CMAKE_${lang}_${var} "${CMAKE_${lang}_${var}}" "${DSYMUTIL_PROGRAM} <TARGET>")
        endforeach()
      endforeach()
    endif()]]
endif()

add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wno-unused-parameter>)
add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wno-missing-field-initializers>)
add_compile_options($<$<COMPILE_LANGUAGE:C>:-Wno-unused-parameter>)
add_compile_options($<$<COMPILE_LANGUAGE:C>:-Wno-missing-field-initializers>)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcolor-diagnostics")
set(CMAKE_CXX_STANDARD "23")
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -D_DEBUG")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fcolor-diagnostics")
set(CMAKE_C_STANDARD "99")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_NAME_DIR "@rpath")

if (TEST_BUILD)
    #add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-O0>)

    # TODO: Investigate why is doesn't work on windows
     
    #add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-fprofile-instr-generate>)
    #add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-fcoverage-mapping>)
    #add_link_options($<$<COMPILE_LANGUAGE:CXX>:-fprofile-instr-generate>)
    #add_link_options($<$<COMPILE_LANGUAGE:CXX>:-fcoverage-mapping>)
    
    #add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-coverage>)
    #add_link_options($<$<COMPILE_LANGUAGE:CXX>:-coverage>)

    #"clang_rt.profile-x86_64.lib"
endif()

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

set(UTILITIES "./utilities/cmake")

foreach(UTILITY IN LISTS UTILITIES)
    file(GLOB UTILITY_FILES "${UTILITY}/*.cmake")

    foreach(UTILITY_FILE IN LISTS UTILITY_FILES)
        include(${UTILITY_FILE})
    endforeach()
endforeach()

add_subdirectory(external)
add_subdirectory(src/Elemental/Common)
add_subdirectory(src/Elemental)
    
if(NOT BUILD_FOR_IOS)
add_subdirectory(src/ElementalTools)
endif()

add_subdirectory(samples)

set(SUB_PROJECTS "./tests" "./samples")

foreach(SUB_PROJECT IN LISTS SUB_PROJECTS)
    file(GLOB_RECURSE SUB_PROJECT_FILES "${SUB_PROJECT}/**/CMakeLists.txt")
    foreach(SUB_PROJECT_FILE IN LISTS SUB_PROJECT_FILES)
        get_filename_component(SUB_PROJECT_DIR ${SUB_PROJECT_FILE} DIRECTORY)
        add_subdirectory(${SUB_PROJECT_DIR})
    endforeach()
endforeach()

# DEBUG Deps
#set_property(GLOBAL PROPERTY GLOBAL_DEPENDS_DEBUG_MODE 1)
