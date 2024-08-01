if(NOT BUILD_FOR_IOS)
    set(SAMPLE_NAME ShaderCompiler)

    add_executable(${SAMPLE_NAME} main.c)
    target_link_libraries(${SAMPLE_NAME} PRIVATE SampleCommon)
    target_link_libraries(${SAMPLE_NAME} PRIVATE ElementalToolsInterface)

    configure_project_package(${SAMPLE_NAME} "samples" DEPENDENCIES ElementalTools)
endif()
