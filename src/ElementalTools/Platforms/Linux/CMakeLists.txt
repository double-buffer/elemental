target_include_directories(ElementalTools PRIVATE ../../../Elemental/Linux/)
target_include_directories(ElementalTools PRIVATE ./Interop/)
target_compile_options(ElementalTools PRIVATE -fms-extensions -Wno-language-extension-token)

set_target_properties(ElementalTools PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/ElementalTools")
set(ELEMENTALTOOLS_OUTPUT_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/ElementalTools/" PARENT_SCOPE)

function(copy_platform_dependencies target)
    copy_dxc_to_target(${target})
endfunction()
