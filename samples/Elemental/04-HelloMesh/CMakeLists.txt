set(SAMPLE_NAME HelloMesh)

#"https://casual-effects.com/g3d/data10/research/model/buddha/buddha.zip"

add_executable(${SAMPLE_NAME} main.c Data)
target_link_libraries(${SAMPLE_NAME} PRIVATE ElementalInterface)

configure_resource_compilation(
    ${SAMPLE_NAME}
    resource_list
    MESH_COMPILER MeshCompiler
    MESH_OUTPUT_EXTENSION .mesh
)
configure_project_package(${SAMPLE_NAME} "samples" DEPENDENCIES Elemental RESOURCES ${resource_list})
