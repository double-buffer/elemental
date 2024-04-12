if(WIN32)
    set(ELEM_PLATFORM_DIR Microsoft)
elseif(APPLE)
    set(ELEM_PLATFORM_DIR Apple)
endif()

add_library(Elemental SHARED ${ELEM_PLATFORM_DIR}/UnityBuild.cpp)
target_include_directories(Elemental PUBLIC .)
set_source_files_properties(ElementalLoader.c PROPERTIES HEADER_FILE_ONLY TRUE)

target_precompile_headers(Elemental PRIVATE ${ELEM_PLATFORM_DIR}/PreCompiledHeader.h)

target_link_libraries(Elemental PRIVATE common)
target_link_libraries(Elemental PRIVATE external_dependencies)

add_subdirectory(${ELEM_PLATFORM_DIR})

copy_platform_dependencies(Elemental)

add_custom_command(TARGET Elemental POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/file.txt
    COMMENT "Touching file.txt after building ProjectA"

)
add_library(ElementalInterface INTERFACE)
target_include_directories(ElementalInterface INTERFACE .)
add_dependencies(ElementalInterface Elemental)
