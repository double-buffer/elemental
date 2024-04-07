add_executable(HelloTriangle Main.c)

target_link_libraries(HelloTriangle PRIVATE ElementalInterface)
target_link_libraries(HelloTriangle PRIVATE ElementalToolsInterface)


#add_executable(HelloTriangle2 MACOSX_BUNDLE Main2.c)
add_executable(HelloTriangle2 Main2.c)

target_link_libraries(HelloTriangle2 PRIVATE ElementalInterface)
target_link_libraries(HelloTriangle2 PRIVATE ElementalToolsInterface)

# TODO: To rework
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Triangle.hlsl
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug/Data/Triangle.hlsl COPYONLY)

