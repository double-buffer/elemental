add_executable(HelloWorld Main.c)

target_compile_options(HelloWorld PRIVATE -W4)
target_compile_options(HelloWorld PRIVATE -Wno-unused-parameter)

target_link_libraries(HelloWorld PRIVATE ElementalHeaders)

## TODO: Refactor that
if(WIN32)
    target_compile_options(HelloWorld PRIVATE -EHs -MD)
    target_compile_definitions(HelloWorld PRIVATE UNICODE _UNICODE _WINDOWS _WIN32)

    add_custom_command(TARGET HelloWorld POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E remove -f "$<TARGET_FILE_DIR:HelloWorld>/*.dll"
        COMMAND ${CMAKE_COMMAND} -E remove -f "$<TARGET_FILE_DIR:HelloWorld>/*.pdb"
        COMMAND ${CMAKE_COMMAND} -E copy_directory $<TARGET_FILE_DIR:Elemental> $<TARGET_FILE_DIR:HelloWorld>)
elseif(APPLE)
    set_target_properties(HelloWorld PROPERTIES
        BUILD_WITH_INSTALL_RPATH TRUE
        INSTALL_NAME_DIR "@rpath"
        INSTALL_RPATH "@loader_path;@loader_path/../lib;$<TARGET_FILE_DIR:Elemental>"
    )
else()
    set_target_properties(HelloWorld PROPERTIES
        INSTALL_RPATH "$ORIGIN;$ORIGIN/../lib;$<TARGET_FILE_DIR:Elemental>"
        BUILD_WITH_INSTALL_RPATH TRUE
    )
endif()
