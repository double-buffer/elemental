include("../common/sample.cmake")

set(SAMPLE_NAME HelloTriangle)

add_executable(${SAMPLE_NAME} Main2.c Data)
target_link_libraries(${SAMPLE_NAME} PRIVATE ElementalInterface)
add_dependencies(${SAMPLE_NAME} Elemental)

configure_resource_compilation(${SAMPLE_NAME} resource_list)

if(APPLE)
    configure_apple_project(${SAMPLE_NAME} ${resource_list})
endif()
