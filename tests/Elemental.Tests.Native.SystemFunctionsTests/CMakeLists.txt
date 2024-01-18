add_executable(Elemental.Tests.NativeSystemFunctionsTests UnityBuild.cpp)

target_compile_options(Elemental.Tests.NativeSystemFunctionsTests PRIVATE -W4)
target_compile_options(Elemental.Tests.NativeSystemFunctionsTests PRIVATE -Wno-unused-parameter)

target_link_libraries(Elemental.Tests.NativeSystemFunctionsTests PRIVATE common)
target_link_libraries(Elemental.Tests.NativeSystemFunctionsTests PRIVATE external_dependencies)
target_link_libraries(Elemental.Tests.NativeSystemFunctionsTests PRIVATE utest)

if(WIN32)
    target_compile_options(Elemental.Tests.NativeSystemFunctionsTests PRIVATE -EHs -MD)
    target_compile_definitions(Elemental.Tests.NativeSystemFunctionsTests PRIVATE UNICODE _UNICODE _WINDOWS _WIN32)
    target_include_directories(Elemental.Tests.NativeSystemFunctionsTests PRIVATE ../../src/Elemental.Native/Windows/)
    target_precompile_headers(Elemental.Tests.NativeSystemFunctionsTests PRIVATE ../../src/Elemental.Native/Windows/PreCompiledHeader.h)
    target_link_libraries(Elemental.Tests.NativeSystemFunctionsTests PRIVATE Dwmapi UxTheme onecore)
elseif(APPLE)
    target_include_directories(Elemental.Tests.NativeSystemFunctionsTests PRIVATE ../../src/Elemental.Native/Apple/)
    target_precompile_headers(Elemental.Tests.NativeSystemFunctionsTests PRIVATE ../../src/Elemental.Native/Apple/PreCompiledHeader.h)
    target_compile_options(Elemental.Tests.NativeSystemFunctionsTests PRIVATE -fms-extensions -fno-objc-arc)

    target_compile_definitions(Elemental.Tests.NativeSystemFunctionsTests PRIVATE __BLOCKS__)
    target_link_options(Elemental.Tests.NativeSystemFunctionsTests PRIVATE "SHELL:-framework Metal" "SHELL:-framework Foundation" "SHELL:-framework Cocoa" "SHELL:-framework CoreGraphics" "SHELL:-framework IOKit")
endif()
