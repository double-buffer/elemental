add_library(Elemental.Native SHARED UnityBuild.cpp)

target_compile_options(Elemental.Native PRIVATE -Wall -fms-extensions -fno-objc-arc)
target_compile_features(Elemental.Native PRIVATE cxx_std_20)
target_precompile_headers(Elemental.Native PRIVATE PrecompiledHeader.h)

target_link_libraries(Elemental.Native PRIVATE common)
target_link_libraries(Elemental.Native PRIVATE external_dependencies)