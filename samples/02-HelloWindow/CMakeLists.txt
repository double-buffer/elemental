include("../common/sample.cmake")

set(SAMPLE_NAME HelloWindow)

add_executable(${SAMPLE_NAME} main.c)
target_link_libraries(${SAMPLE_NAME} PRIVATE ElementalInterface)
add_dependencies(${SAMPLE_NAME} Elemental)

configure_project_package(${SAMPLE_NAME} "")
