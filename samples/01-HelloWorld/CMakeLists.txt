add_executable(HelloWorld Main.c)

set_target_properties(HelloWorld PROPERTIES LINKER_LANGUAGE C)

target_compile_options(HelloWorld PRIVATE -W4)
target_compile_options(HelloWorld PRIVATE -Wno-unused-parameter)

target_link_libraries(HelloWorld INTERFACE Elemental)

if(WIN32)
    target_compile_options(HelloWorld PRIVATE -EHs -MD)
    target_compile_definitions(HelloWorld PRIVATE UNICODE _UNICODE _WINDOWS _WIN32)
elseif(APPLE)
endif()
