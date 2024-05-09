add_executable(ToolsTests UnityBuild.cpp)
target_link_libraries(ToolsTests PRIVATE utest)
target_link_libraries(ToolsTests PRIVATE ElementalToolsInterface)

configure_project_package(ToolsTests "tests" DEPENDENCIES ElementalTools)
