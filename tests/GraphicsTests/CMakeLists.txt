add_executable(GraphicsTests UnityBuild.cpp Data)
target_link_libraries(GraphicsTests PRIVATE utest)
target_link_libraries(GraphicsTests PRIVATE ElementalInterface)

configure_resource_compilation(GraphicsTests resource_list)
configure_project_package(GraphicsTests "tests" DEPENDENCIES Elemental RESOURCES ${resource_list})
