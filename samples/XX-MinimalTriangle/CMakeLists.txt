set(SAMPLE_NAME MinimalHelloTriangle)

add_executable(${SAMPLE_NAME} main.c)
target_link_libraries(${SAMPLE_NAME} PRIVATE ElementalInterface)
target_link_libraries(${SAMPLE_NAME} PRIVATE ElementalToolsInterface)

configure_project_package(${SAMPLE_NAME} "samples" DEPENDENCIES Elemental ElementalTools)
