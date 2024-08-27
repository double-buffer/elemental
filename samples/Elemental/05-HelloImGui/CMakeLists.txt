set(SAMPLE_NAME HelloImGui)

add_executable(${SAMPLE_NAME} main.c Data)
target_link_libraries(${SAMPLE_NAME} PRIVATE SampleCommon)
target_link_libraries(${SAMPLE_NAME} PUBLIC cimgui)
target_link_libraries(${SAMPLE_NAME} PRIVATE ElementalInterface)
add_dependencies(${SAMPLE_NAME} SampleSharedData)

configure_resource_compilation(${SAMPLE_NAME} resource_list)
configure_project_package(${SAMPLE_NAME} "samples" DEPENDENCIES Elemental RESOURCES ${resource_list})
