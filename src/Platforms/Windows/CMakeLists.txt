add_library(Elemental.Native SHARED UnityBuild.cpp)

target_compile_options(Elemental.Native PRIVATE -Wall -EHs -MD)
target_compile_features(Elemental.Native PRIVATE cxx_std_20)
target_precompile_headers(Elemental.Native PRIVATE PrecompiledHeader.h)

target_link_libraries(Elemental.Native PRIVATE common)
target_compile_definitions(Elemental.Native PRIVATE UNICODE _UNICODE)

#set_property(TARGET Elemental.Native PROPERTY VS_PACKAGE_REFERENCES "Microsoft.Direct3D.D3D12_1.610.0")

# https://www.nuget.org/api/v2/package/Microsoft.Direct3D.D3D12/1.610.2