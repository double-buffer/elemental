#add_executable(HelloTriangle Main.c)

#target_link_libraries(HelloTriangle PRIVATE ElementalInterface)
#target_link_libraries(HelloTriangle PRIVATE ElementalToolsInterface)

if (APPLE)
    if(BUILD_FOR_IOS)
        add_executable(HelloTriangle2 MACOSX_BUNDLE Main2.c)
        set_target_properties(HelloTriangle2 PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/../Common/iOS/Info.plist)
    else()
        #add_executable(HelloTriangle2 MACOSX_BUNDLE Main2.c)
        add_executable(HelloTriangle2 Main2.c)
        target_link_libraries(HelloTriangle2 PRIVATE ElementalToolsInterface)
        target_link_libraries(HelloTriangle2 PRIVATE Elemental)
    endif()
    
    set(RESOURCE_FILES
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug/Data/Triangle.bin")

    ## TODO: Find another way to get the framework path
    set_target_properties(HelloTriangle2 PROPERTIES
        XCODE_EMBED_FRAMEWORKS "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug/Elemental.framework"
        XCODE_EMBED_FRAMEWORKS_CODE_SIGN_ON_COPY		"YES"
        XCODE_EMBED_FRAMEWORKS_REMOVE_HEADERS_ON_COPY	"YES"
        RESOURCE "${RESOURCE_FILES}"
    )

else()
    add_executable(HelloTriangle2 Main2.c)
    target_link_libraries(HelloTriangle2 PRIVATE ElementalToolsInterface)
endif()


target_link_libraries(HelloTriangle2 PRIVATE ElementalInterface)

add_custom_command(OUTPUT "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug/Data/Triangle.bin"
    COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug/ShaderCompiler "${CMAKE_CURRENT_SOURCE_DIR}/Triangle.hlsl" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug/Data/Triangle.bin"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/Triangle.hlsl"
    COMMENT "Compiling Triangle.hlsl to Triangle.bin")

add_custom_target(CompileShaders ALL
    DEPENDS "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug/Data/Triangle.bin"
    COMMENT "Ensuring HLSL shaders are compiled...")

add_dependencies(CompileShaders ShaderCompiler)

add_dependencies(HelloTriangle2 CompileShaders)
