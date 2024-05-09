add_executable(GraphicsTests UnityBuild.cpp)
target_link_libraries(GraphicsTests PRIVATE utest)
target_link_libraries(GraphicsTests PRIVATE ElementalInterface)

configure_project_package(GraphicsTests "tests" DEPENDENCIES Elemental)
