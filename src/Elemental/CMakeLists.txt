if(WIN32)
    set(ELEM_PLATFORM_DIR Microsoft)
elseif(APPLE)
    set(ELEM_PLATFORM_DIR Apple)
elseif(LINUX)
    set(ELEM_PLATFORM_DIR Linux)
endif()

add_library(Elemental SHARED ${ELEM_PLATFORM_DIR}/UnityBuild.cpp)
set_target_properties(Elemental PROPERTIES
    LANGUAGE CXX
    LINKER_LANGUAGE CXX
)
target_include_directories(Elemental PUBLIC .)
target_precompile_headers(Elemental PRIVATE ${ELEM_PLATFORM_DIR}/PreCompiledHeader.h)
target_link_libraries(Elemental PRIVATE elemental_common foundations external_dependencies)

add_subdirectory(${ELEM_PLATFORM_DIR})
copy_platform_dependencies(Elemental)

add_library(ElementalInterface INTERFACE)
target_include_directories(ElementalInterface INTERFACE .)
add_dependencies(ElementalInterface Elemental)

install(
    DIRECTORY ${ELEMENTAL_OUTPUT_DIR}
    DESTINATION Elemental/bin
    USE_SOURCE_PERMISSIONS
)

install(
    FILES Elemental.h ElementalLoader.c
    DESTINATION Elemental/include
)

install(
    FILES ../../LICENSE
    DESTINATION Elemental
)
