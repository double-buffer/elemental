if(WIN32)
    set(ELEM_PLATFORM_DIR Microsoft)
elseif(APPLE)
    set(ELEM_PLATFORM_DIR Apple)
elseif(LINUX)
    set(ELEM_PLATFORM_DIR Linux)
endif()

add_library(ElementalTools SHARED Common/UnityBuild.cpp)
target_include_directories(ElementalTools PUBLIC .)

target_precompile_headers(ElementalTools PRIVATE ${ELEM_PLATFORM_DIR}/PreCompiledHeader.h)

target_link_libraries(ElementalTools PRIVATE common)
target_link_libraries(ElementalTools PRIVATE external_dependencies)
target_link_libraries(ElementalTools PRIVATE tools_external_dependencies)

add_subdirectory(${ELEM_PLATFORM_DIR})

copy_platform_dependencies(ElementalTools)

add_library(ElementalToolsInterface INTERFACE)
target_include_directories(ElementalToolsInterface INTERFACE .)
add_dependencies(ElementalToolsInterface ElementalTools)
