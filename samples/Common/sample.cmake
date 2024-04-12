function(configure_resource_compilation target_name resource_list)
    if(BUILD_FOR_IOS)
        set(SHADER_COMPILER_BIN "${CMAKE_SOURCE_DIR}/build/bin/ShaderCompiler")
        set(SHADER_COMPILER_OPTIONS "--target-platform" "iOS")
    else()
        if(CMAKE_GENERATOR STREQUAL "Ninja")
            set(SHADER_COMPILER_BIN "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/ShaderCompiler")
        else()
            set(SHADER_COMPILER_BIN "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug/ShaderCompiler")
        endif()
        set(SHADER_COMPILER_OPTIONS "")
        add_dependencies(${target_name} ShaderCompiler)
    endif()

    # TODO: Take into account other type of resources
    set(hlsl_files_list "")
    set(compiled_shaders "")

    get_target_property(target_sources ${target_name} SOURCES)

    foreach(source IN LISTS target_sources)
        if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${source})
            file(GLOB_RECURSE directory_hlsl_files "${CMAKE_CURRENT_SOURCE_DIR}/${source}/*.hlsl")
            list(APPEND hlsl_files_list ${directory_hlsl_files})
        else()
            get_filename_component(ext ${source} EXT)
            if("${ext}" STREQUAL ".hlsl")
                list(APPEND hlsl_files_list ${CMAKE_CURRENT_SOURCE_DIR}/${source})
            endif()
        endif()
    endforeach()

    foreach(hlsl_file IN LISTS hlsl_files_list)
        get_filename_component(file_path ${hlsl_file} ABSOLUTE)
        get_filename_component(file_dir ${file_path} DIRECTORY)
        get_filename_component(file_name ${hlsl_file} NAME_WE)

        file(RELATIVE_PATH relative_dir ${CMAKE_CURRENT_SOURCE_DIR} ${file_dir})
        set(output_dir "${CMAKE_CURRENT_BINARY_DIR}/${relative_dir}")
        file(MAKE_DIRECTORY ${output_dir})

        set(compiled_shader "${output_dir}/${file_name}.bin")

        add_custom_command(OUTPUT ${compiled_shader}
            COMMAND ${SHADER_COMPILER_BIN} ${SHADER_COMPILER_OPTIONS} ${hlsl_file} ${compiled_shader}
            DEPENDS ${hlsl_file}
            COMMENT "Compiling HLSL shader: ${hlsl_file}"
            WORKING_DIRECTORY ${file_dir}
        )

        set_source_files_properties(${compiled_shader} PROPERTIES GENERATED TRUE)
        target_sources(${target_name} PRIVATE ${compiled_shader})

        list(APPEND compiled_shaders ${compiled_shader})
    endforeach()

    set(${resource_list} "${compiled_shaders}" PARENT_SCOPE)
endfunction()

function(configure_apple_project target_name resource_list)
    if(APPLE)
        set_target_properties(${target_name} PROPERTIES 
            MACOSX_BUNDLE "TRUE"
            RESOURCE ${resource_list}    
        )

        if(CMAKE_GENERATOR STREQUAL "Xcode")
            set(ELEMENTAL_PATH "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug/Elemental.framework")

            set_target_properties(${target_name} PROPERTIES
                XCODE_EMBED_FRAMEWORKS ${ELEMENTAL_PATH}
                XCODE_EMBED_FRAMEWORKS_CODE_SIGN_ON_COPY "YES"
                XCODE_EMBED_FRAMEWORKS_REMOVE_HEADERS_ON_COPY "YES"
            )
        else()
            set(ELEMENTAL_PATH "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Elemental.framework")

            get_filename_component(FRAMEWORK_NAME "${ELEMENTAL_PATH}" NAME)
            set(framework_destination "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${target_name}.app/Contents/Frameworks/${FRAMEWORK_NAME}")

            add_custom_target(CopyFrameworkFolder ALL)

            add_custom_command(TARGET CopyFrameworkFolder POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory "${framework_destination}"
                COMMAND ${CMAKE_COMMAND} -E copy_directory "${ELEMENTAL_PATH}" "${framework_destination}"
                COMMENT "Copying framework folder to destination"
            )

            add_dependencies(CopyFrameworkFolder Elemental)
            add_dependencies(${target_name} CopyFrameworkFolder)
        endif()
        
        if(BUILD_FOR_IOS)
            set_target_properties(${target_name} PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/../Common/iOS/Info.plist)
        endif()
    endif()
endfunction()

