target_compile_options(Elemental PRIVATE -EHs)

target_link_libraries(Elemental PRIVATE Direct3D12)

target_link_libraries(Elemental PRIVATE Dwmapi UxTheme onecore)
target_compile_definitions(Elemental PRIVATE UNICODE _UNICODE _WINDOWS _WIN32)
        
set_target_properties(Elemental PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Elemental")
set(ELEMENTAL_OUTPUT_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Elemental/" PARENT_SCOPE)

function(copy_platform_dependencies target)
    copy_direct3d12_to_target(${target})

    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Elemental" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
        COMMENT "Copy Elemental Binaries"
    )
endfunction()
