
if(WIN32)
    set(ELEM_PLATFORM_DIR Microsoft)
elseif(APPLE)
    set(ELEM_PLATFORM_DIR Apple)
endif()

add_library(Elemental.Tools SHARED Common/UnityBuild.cpp)
target_include_directories(Elemental.Tools PUBLIC .)

target_precompile_headers(Elemental.Tools PRIVATE ${ELEM_PLATFORM_DIR}/PreCompiledHeader.h)

target_link_libraries(Elemental.Tools PRIVATE common)
target_link_libraries(Elemental.Tools PRIVATE external_dependencies)
target_link_libraries(Elemental.Tools PRIVATE tools_external_dependencies)

add_subdirectory(${ELEM_PLATFORM_DIR})

copy_platform_dependencies(Elemental.Tools)

add_library(ElementalToolsInterface INTERFACE)
target_include_directories(ElementalToolsInterface INTERFACE .)
