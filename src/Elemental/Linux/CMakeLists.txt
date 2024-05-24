target_compile_options(Elemental PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wno-nullability-extension>)
target_compile_options(Elemental PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wno-c11-extensions>)

target_link_libraries(Elemental PRIVATE Wayland)

set_target_properties(Elemental PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Elemental")
set(ELEMENTAL_OUTPUT_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Elemental/" PARENT_SCOPE)

function(copy_platform_dependencies target)
endfunction()
