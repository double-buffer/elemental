aux_source_directory(. SRC_LIST)
add_executable(Elemental.Tests.NativeSystemFunctionsTests ${SRC_LIST})

target_compile_options(Elemental.Tests.NativeSystemFunctionsTests PRIVATE -W4)
target_compile_options(Elemental.Tests.NativeSystemFunctionsTests PRIVATE -Wno-unused-parameter)
target_compile_features(Elemental.Tests.NativeSystemFunctionsTests PRIVATE cxx_std_20)

target_link_libraries(Elemental.Tests.NativeSystemFunctionsTests PRIVATE common)
target_link_libraries(Elemental.Tests.NativeSystemFunctionsTests PRIVATE utest)

if(WIN32)
    target_compile_options(Elemental.Tests.NativeSystemFunctionsTests PRIVATE -EHs -MD)
    target_compile_definitions(Elemental.Tests.NativeSystemFunctionsTests PRIVATE UNICODE _UNICODE _WINDOWS _WIN32)
endif()
