add_library(Elemental SHARED UnityBuild.cpp)

target_include_directories(Elemental INTERFACE ../.)

target_compile_options(Elemental PRIVATE -Wall -fno-objc-arc)
target_include_directories(Elemental PRIVATE .)
target_precompile_headers(Elemental PRIVATE PreCompiledHeader.h)

target_link_libraries(Elemental PRIVATE common)
target_link_libraries(Elemental PRIVATE external_dependencies)

target_compile_definitions(Elemental PRIVATE __BLOCKS__)
target_link_options(Elemental PRIVATE "SHELL:-framework Metal" "SHELL:-framework Foundation" "SHELL:-framework Cocoa" "SHELL:-framework CoreGraphics" "SHELL:-framework IOKit")
