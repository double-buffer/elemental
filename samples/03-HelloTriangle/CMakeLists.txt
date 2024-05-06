include("../Common/sample.cmake")

set(SAMPLE_NAME HelloTriangle)

add_executable(${SAMPLE_NAME} main.c Data)
target_link_libraries(${SAMPLE_NAME} PRIVATE ElementalInterface)
add_dependencies(${SAMPLE_NAME} Elemental)

configure_resource_compilation(${SAMPLE_NAME} resource_list)
configure_project_package(${SAMPLE_NAME} RESOURCE_LIST ${resource_list})
