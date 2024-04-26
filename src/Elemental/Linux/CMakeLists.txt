target_compile_options(Elemental PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wno-nullability-extension>)
target_compile_options(Elemental PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wno-c11-extensions>)

function(copy_platform_dependencies target)
endfunction()
