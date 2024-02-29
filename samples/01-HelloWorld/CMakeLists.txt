add_executable(HelloWorld Main.c)

target_link_libraries(HelloWorld PRIVATE Elemental)

## TODO: Refactor that
if(WIN32)
    #TODO:  I think we don't need that
    target_compile_options(HelloWorld PRIVATE -EHs -MD)
    target_compile_definitions(HelloWorld PRIVATE UNICODE _UNICODE _WINDOWS _WIN32)
endif()
