if(NOT BUILD_FOR_IOS)
    add_executable(ToolsTests UnityBuild.cpp)
    target_link_libraries(ToolsTests PRIVATE utest)
    target_link_libraries(ToolsTests PRIVATE ElementalToolsInterface)

    configure_project_package(ToolsTests "tests" DEPENDENCIES ElementalTools)
endif()
