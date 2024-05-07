if(WIN32)
    set(ELEM_PLATFORM_DIR Microsoft)
elseif(APPLE)
    set(ELEM_PLATFORM_DIR Apple)
elseif(LINUX)
    set(ELEM_PLATFORM_DIR Linux)
endif()

add_library(Elemental SHARED ${ELEM_PLATFORM_DIR}/UnityBuild.cpp)
set_target_properties(Elemental PROPERTIES LANGUAGE CXX)
set_target_properties(Elemental PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(Elemental PUBLIC .)

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

get_target_property(ELEMENTAL_OUTPUT_DIR Elemental RUNTIME_OUTPUT_DIRECTORY)

install(DIRECTORY ${ELEMENTAL_OUTPUT_DIR}/
        DESTINATION Elemental/bin
)

install(FILES Elemental.h ElementalLoader.c
        DESTINATION Elemental/include
)

install(FILES ../../LICENSE
        DESTINATION Elemental
)

install(DIRECTORY ${ELEMENTAL_OUTPUT_DIR}/
        DESTINATION tests
)
