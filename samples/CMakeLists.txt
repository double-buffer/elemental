add_subdirectory(01-HelloWindow)

if(NOT BUILD_FOR_IOS)
    add_subdirectory(02-ShaderCompiler)
endif()

add_subdirectory(03-HelloTriangle)

set(APPFOLDER_EXTENSION "")

if (APPLE)
    set(APPFOLDER_EXTENSION ".app")
endif()

install(DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/HelloWindow${APPFOLDER_EXTENSION}
        DESTINATION samples
        USE_SOURCE_PERMISSIONS
)

install(DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/HelloTriangle${APPFOLDER_EXTENSION}
        DESTINATION samples
        USE_SOURCE_PERMISSIONS
)
