add_executable(SystemFunctionsTests UnityBuild.cpp)

target_link_libraries(SystemFunctionsTests PRIVATE common)
target_link_libraries(SystemFunctionsTests PRIVATE external_dependencies)
target_link_libraries(SystemFunctionsTests PRIVATE utest)
target_link_libraries(SystemFunctionsTests PRIVATE ElementalInterface)

if(WIN32)
    target_compile_options(SystemFunctionsTests PRIVATE -EHs -MD)
    target_compile_definitions(SystemFunctionsTests PRIVATE UNICODE _UNICODE _WINDOWS _WIN32)
    target_include_directories(SystemFunctionsTests PRIVATE ../../src/Elemental/Microsoft/)
    target_precompile_headers(SystemFunctionsTests PRIVATE ../../src/Elemental/Microsoft/PreCompiledHeader.h)
    target_link_libraries(SystemFunctionsTests PRIVATE Dwmapi UxTheme onecore)
elseif(APPLE)
    target_include_directories(SystemFunctionsTests PRIVATE ../../src/Elemental/Apple/)
    target_precompile_headers(SystemFunctionsTests PRIVATE ../../src/Elemental/Apple/PreCompiledHeader.h)
    target_compile_options(SystemFunctionsTests PRIVATE -fms-extensions -fno-objc-arc)

    target_compile_definitions(SystemFunctionsTests PRIVATE __BLOCKS__)

    if(BUILD_FOR_IOS)
        target_link_options(SystemFunctionsTests PRIVATE "SHELL:-framework Metal" "SHELL:-framework QuartzCore" "SHELL:-framework Foundation" "SHELL:-framework UIKit" "SHELL:-framework CoreGraphics" "SHELL:-framework IOKit")
    else()
        target_link_options(SystemFunctionsTests PRIVATE "SHELL:-framework Metal" "SHELL:-framework QuartzCore" "SHELL:-framework Foundation" "SHELL:-framework Cocoa" "SHELL:-framework CoreGraphics" "SHELL:-framework IOKit")
    endif()
elseif(LINUX)
    target_include_directories(SystemFunctionsTests PRIVATE ../../src/Elemental/Linux/)
    target_precompile_headers(SystemFunctionsTests PRIVATE ../../src/Elemental/Linux/PreCompiledHeader.h)
    target_compile_options(SystemFunctionsTests PRIVATE -fms-extensions)

    target_link_libraries(SystemFunctionsTests PRIVATE Wayland)
endif()

configure_project_package(SystemFunctionsTests "tests")
