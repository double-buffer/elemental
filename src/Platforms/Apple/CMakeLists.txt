add_library(Elemental.Native SHARED UnityBuild.cpp)

target_compile_options(Elemental.Native PRIVATE -Wall -fno-objc-arc)
target_compile_features(Elemental.Native PRIVATE cxx_std_20)
target_precompile_headers(Elemental.Native PRIVATE PreCompiledHeader.h)

target_link_libraries(Elemental.Native PRIVATE common)
target_link_libraries(Elemental.Native PRIVATE external_dependencies)

target_link_options(Elemental.Native PRIVATE "SHELL:-framework Metal" "SHELL:-framework Foundation" "SHELL:-framework Cocoa" "SHELL:-framework CoreGraphics")