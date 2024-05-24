set(SAMPLE_NAME HelloInputs)

add_executable(${SAMPLE_NAME} main.c Data)
target_link_libraries(${SAMPLE_NAME} PRIVATE ElementalInterface)

configure_resource_compilation(${SAMPLE_NAME} resource_list)
configure_project_package(${SAMPLE_NAME} "samples" DEPENDENCIES Elemental RESOURCES ${resource_list})
