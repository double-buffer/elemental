add_subdirectory(01-HelloWindow)

if(NOT BUILD_FOR_IOS)
    add_subdirectory(02-ShaderCompiler)
endif()

add_subdirectory(03-HelloTriangle)

install(DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/HelloWindow
        DESTINATION samples
)

install(DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/HelloTriangle
        DESTINATION samples
)
