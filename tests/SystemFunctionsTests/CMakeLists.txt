add_executable(SystemFunctionsTests UnityBuild.cpp)

target_compile_options(SystemFunctionsTests PRIVATE -W4)
target_compile_options(SystemFunctionsTests PRIVATE -Wno-unused-parameter)

target_link_libraries(SystemFunctionsTests PRIVATE common)
target_link_libraries(SystemFunctionsTests PRIVATE external_dependencies)
target_link_libraries(SystemFunctionsTests PRIVATE utest)

if(WIN32)
    target_compile_options(SystemFunctionsTests PRIVATE -EHs -MD)
    target_compile_definitions(SystemFunctionsTests PRIVATE UNICODE _UNICODE _WINDOWS _WIN32)
    target_include_directories(SystemFunctionsTests PRIVATE ../../src/Elemental.Native/Windows/)
    target_precompile_headers(SystemFunctionsTests PRIVATE ../../src/Elemental.Native/Windows/PreCompiledHeader.h)
    target_link_libraries(SystemFunctionsTests PRIVATE Dwmapi UxTheme onecore)
elseif(APPLE)
    target_include_directories(SystemFunctionsTests PRIVATE ../../src/Elemental.Native/Apple/)
    target_precompile_headers(SystemFunctionsTests PRIVATE ../../src/Elemental.Native/Apple/PreCompiledHeader.h)
    target_compile_options(SystemFunctionsTests PRIVATE -fms-extensions -fno-objc-arc)

    target_compile_definitions(SystemFunctionsTests PRIVATE __BLOCKS__)
    target_link_options(SystemFunctionsTests PRIVATE "SHELL:-framework Metal" "SHELL:-framework Foundation" "SHELL:-framework Cocoa" "SHELL:-framework CoreGraphics" "SHELL:-framework IOKit")
endif()
