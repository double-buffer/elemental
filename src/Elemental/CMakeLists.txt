if(WIN32)
    set(ELEM_PLATFORM_DIR Windows)
elseif(APPLE)
    set(ELEM_PLATFORM_DIR Apple)
endif()

add_library(Elemental SHARED ${ELEM_PLATFORM_DIR}/UnityBuild.cpp)
target_include_directories(Elemental PUBLIC .)

target_precompile_headers(Elemental PRIVATE ${ELEM_PLATFORM_DIR}/PreCompiledHeader.h)

target_link_libraries(Elemental PRIVATE common)
target_link_libraries(Elemental PRIVATE external_dependencies)

add_subdirectory(${ELEM_PLATFORM_DIR})
