if(NOT BUILD_FOR_IOS)
    add_executable(ShaderCompiler main.c)
    target_link_libraries(ShaderCompiler PRIVATE ElementalToolsInterface)

    configure_project_package(ShaderCompiler "samples" DEPENDENCIES ElementalTools)
endif()
