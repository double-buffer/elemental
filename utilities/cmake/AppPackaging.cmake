function(get_compiler_bin_path binary_name out_var)
    if(BUILD_FOR_IOS)
        set(bin_path "${CMAKE_SOURCE_DIR}/build/bin/${binary_name}.app/Contents/MacOS/${binary_name}")
    else()
        if(CMAKE_GENERATOR STREQUAL "Ninja")
            if(APPLE)
                set(bin_path "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${binary_name}.app/Contents/MacOS/${binary_name}")
            else()
                set(bin_path "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${binary_name}/${binary_name}")
            endif()
        else()
            set(bin_path "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug/${binary_name}")
        endif()
    endif()
    set(${out_var} "${bin_path}" PARENT_SCOPE)
endfunction()


function(configure_resources_for_compiler
    target_name         # e.g. MyApp
    result_var         # e.g. compiled_files_for_X
    name               # e.g. HLSL
    binary_name        # e.g. ShaderCompiler
    source_exts        # ".hlsl"
    dest_ext           # ".shader"
    resource_root      # the folder from resource_list if it is a directory
    )
    # Convert commas to semicolons if present
    string(REPLACE "," ";" source_exts "${source_exts}")

    get_target_property(target_sources ${target_name} SOURCES)

    set(resource_files "")
    foreach(src IN LISTS target_sources)
        # Combine the "folder or file" from resource_root with the target source
        # if your calling code is using it that way. (This part might vary depending on your usage.)
        set(abs_src "${CMAKE_CURRENT_SOURCE_DIR}/${src}")

        # If we are dealing with a directory in resource_root or if 'src' *is* that directory,
        # then GLOB_RECURSE subfiles
        if(IS_DIRECTORY "${abs_src}")
            foreach(ext IN LISTS source_exts)
                file(GLOB_RECURSE matched_files "${abs_src}/*${ext}")
                list(APPEND resource_files ${matched_files})
            endforeach()
        else()
            # It's a single file—check extension
            get_filename_component(this_ext "${abs_src}" EXT)
            foreach(ext IN LISTS source_exts)
                if("${this_ext}" STREQUAL "${ext}")
                    list(APPEND resource_files "${abs_src}")
                    break()
                endif()
            endforeach()
        endif()
    endforeach()

    if(resource_files STREQUAL "")
        set(${result_var} "" PARENT_SCOPE)
        return()
    endif()

    # Use the new param as the "root" for the relative subfolder structure
    # If resource_root is empty or not a directory, you might choose a fallback:
    if(NOT IS_DIRECTORY "${resource_root}")
        # fallback if needed
        set(resource_root "${CMAKE_CURRENT_SOURCE_DIR}")
    endif()

    # Get path to the compiler binary
    get_compiler_bin_path(${binary_name} compiler_bin)

    # Grab any remaining ARGN as default parameters
    # (If you’re using ARGN for something else, adapt accordingly.)
    set(args_to_parse ${ARGN})
    set(default_params "")
    foreach(param IN LISTS args_to_parse)
        if(NOT param STREQUAL "")
            list(APPEND default_params ${param})
        endif()
    endforeach()

    set(compiled_files "")

    foreach(file IN LISTS resource_files)
        get_filename_component(file_path "${file}" ABSOLUTE)
        get_filename_component(file_dir  "${file_path}" DIRECTORY)
        get_filename_component(file_name "${file_path}" NAME_WE)

        # Compute the relative subfolder structure from 'resource_root'
        file(RELATIVE_PATH relative_dir "${resource_root}" "${file_dir}")

          # Reproduce that structure inside the build folder
        set(output_dir "${CMAKE_CURRENT_BINARY_DIR}/${relative_dir}")
        file(MAKE_DIRECTORY "${output_dir}")

        set(output_file "${output_dir}/${file_name}${dest_ext}")

        add_custom_command(
            OUTPUT  "${output_file}"
            COMMAND "${compiler_bin}" ${default_params} "${file}" "${output_file}"
            DEPENDS "${file}"
            COMMENT "Compiling ${name}: ${file}"
            WORKING_DIRECTORY "${file_dir}"
        )

        set_source_files_properties("${output_file}" PROPERTIES GENERATED TRUE)
        target_sources("${target_name}" PRIVATE "${output_file}")
        list(APPEND compiled_files "${output_file}")

        # Extra Vulkan variant for HLSL on Windows
        if("${name}" STREQUAL "HLSL" AND WIN32)
            set(vulkan_output_file "${output_dir}/${file_name}_vulkan${dest_ext}")
            add_custom_command(
                OUTPUT  "${vulkan_output_file}"
                COMMAND "${compiler_bin}" ${default_params} --target-api vulkan "${file}" "${vulkan_output_file}"
                DEPENDS "${file}"
                COMMENT "Compiling ${name} (Vulkan): ${file}"
                WORKING_DIRECTORY "${file_dir}"
            )
            set_source_files_properties("${vulkan_output_file}" PROPERTIES GENERATED TRUE)
            target_sources("${target_name}" PRIVATE "${vulkan_output_file}")
            list(APPEND compiled_files "${vulkan_output_file}")
        endif()
    endforeach()

    set(${result_var} "${compiled_files}" PARENT_SCOPE)
endfunction()


function(configure_resource_compilation target_name resource_list)
    # iOS, debug logic, etc.
    if(BUILD_FOR_IOS)
        set(SHADER_COMPILER_DEFAULT_OPTIONS "--target-platform iOS")
    else()
        set(SHADER_COMPILER_DEFAULT_OPTIONS "")
        add_dependencies(${target_name} ShaderCompiler)
    endif()

    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        if(NOT "${SHADER_COMPILER_DEFAULT_OPTIONS}" STREQUAL "")
            set(SHADER_COMPILER_DEFAULT_OPTIONS "${SHADER_COMPILER_DEFAULT_OPTIONS} --debug")
        else()
            set(SHADER_COMPILER_DEFAULT_OPTIONS "--debug")
        endif()
    endif()

    set(MESH_COMPILER_DEFAULT_OPTIONS "")
    set(TEXTURE_COMPILER_DEFAULT_OPTIONS "")

    # Use '|' as delimiters
    set(COMPILERS_LIST
        "HLSL|ShaderCompiler|.hlsl|.shader|${SHADER_COMPILER_DEFAULT_OPTIONS}"
        "MESH|SceneCompiler|.obj|.scene|${MESH_COMPILER_DEFAULT_OPTIONS}"
        "MESH|SceneCompiler|.gltf|.scene|${MESH_COMPILER_DEFAULT_OPTIONS}"
        "TEXTURE|TextureCompiler|.tga|.texture|${TEXTURE_COMPILER_DEFAULT_OPTIONS}"
    )

    set(all_compiled_resources "")
    foreach(compiler_entry IN LISTS COMPILERS_LIST)
        string(REPLACE "|" ";" fields_string "${compiler_entry}")
        set(fields ${fields_string})
        list(LENGTH fields fields_length)
        if(fields_length LESS 4)
            message(FATAL_ERROR "Invalid compiler definition: ${compiler_entry}")
        endif()

        list(GET fields 0 name)
        list(GET fields 1 binary_name)
        list(GET fields 2 source_exts)
        list(GET fields 3 dest_ext)

        if(fields_length GREATER 4)
            list(SUBLIST fields 4 -1 default_params)
        else()
            set(default_params "")
        endif()

        #-----------------------------------------------------------------
        # Here we call our new configure_resources_for_compiler.
        # The last param is the folder from resource_list if you want
        # to treat it as the "root" for preserving subfolders.
        #
        # You could pass `resource_list` directly. If your `resource_list`
        # has multiple entries, you might loop them or parse them individually.
        #-----------------------------------------------------------------
        configure_resources_for_compiler(
            "${target_name}"
            compiled_files_for_${name}
            "${name}"
            "${binary_name}"
            "${source_exts}"
            "${dest_ext}"
            "${resource_list}"  # TODO: We can remove that parameter
            ${default_params}
        )

        list(APPEND all_compiled_resources ${compiled_files_for_${name}})
    endforeach()

    set(${resource_list} "${all_compiled_resources}" PARENT_SCOPE)
endfunction()



function(configure_project_package target_name install_folder)
    cmake_parse_arguments(ARG "" "" "DEPENDENCIES;RESOURCES" ${ARGV})

    list(LENGTH ARG_DEPENDENCIES dependencies_length)
    # message("Number of Dependencies: ${dependencies_length}")
            
    list(LENGTH ARG_RESOURCES resources_length)
    # message("Number of resources: ${resources_length}")

    if(APPLE)
        set(APPFOLDER_EXTENSION ".app")

        set_target_properties(${target_name} PROPERTIES 
            MACOSX_BUNDLE "TRUE"
        )

        # TODO: Use dependencies variable instead
        if(CMAKE_GENERATOR STREQUAL "Xcode")
            set(ELEMENTAL_PATH "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug/Elemental.framework")

            set_target_properties(${target_name} PROPERTIES
                XCODE_EMBED_FRAMEWORKS ${ELEMENTAL_PATH}
                XCODE_EMBED_FRAMEWORKS_CODE_SIGN_ON_COPY "YES"
                XCODE_EMBED_FRAMEWORKS_REMOVE_HEADERS_ON_COPY "YES"
            )

            foreach(res IN LISTS ARG_RESOURCES)
                get_filename_component(file_name "${res}" NAME)
                get_filename_component(full_path "${res}" ABSOLUTE)
                get_filename_component(file_dir "${full_path}" DIRECTORY)

                set(resource_root "${CMAKE_CURRENT_BINARY_DIR}/Data") 

                file(RELATIVE_PATH relative_dir "${resource_root}" "${file_dir}")

                set_source_files_properties("${full_path}"
                    PROPERTIES MACOSX_PACKAGE_LOCATION "${relative_dir}"
                )

                # Attach file to the target so Xcode knows to include it
                target_sources(${target_name} PRIVATE "${full_path}")
            endforeach()
        else()
            add_custom_target(CopyFrameworkFolder${target_name} ALL)

            foreach(dependency IN LISTS ARG_DEPENDENCIES)
                message("Dep: ${dependency}")
                add_dependencies(CopyFrameworkFolder${target_name} ${dependency})
            
                set(ELEMENTAL_PATH "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${dependency}.framework")
                get_filename_component(FRAMEWORK_NAME "${ELEMENTAL_PATH}" NAME)
                set(framework_destination "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${target_name}.app/Contents/Frameworks/${FRAMEWORK_NAME}")
            
                # TODO: Remove also headers in the version directory
                add_custom_command(TARGET CopyFrameworkFolder${target_name} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E make_directory "${framework_destination}"
                    COMMAND cp -RP "${ELEMENTAL_PATH}/" "${framework_destination}/"
                    COMMAND ${CMAKE_COMMAND} -E remove_directory "${framework_destination}/Headers"
                    #COMMAND ${CMAKE_COMMAND} -E remove_directory "${framework_destination}/Versions"
                    COMMENT "Copying ${dependency} framework folder to destination"
                )
            endforeach()
        
            if(NOT resources_length EQUAL 0)
                set(output_folder "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${target_name}.app/Contents/")

                foreach(file IN LISTS ARG_RESOURCES)
                    get_filename_component(file_name "${file}" NAME)
                    get_filename_component(full_path "${file}" ABSOLUTE)
                    get_filename_component(file_dir "${full_path}" DIRECTORY)

                    set(resource_root "${CMAKE_CURRENT_BINARY_DIR}/Data") 

                    file(RELATIVE_PATH relative_dir "${resource_root}" "${file_dir}")
                    set(output_subdir "${output_folder}/Resources/${relative_dir}")
                    file(MAKE_DIRECTORY "${output_subdir}")

                    set(output_file "${output_subdir}/${file_name}")

                    add_custom_command(
                    OUTPUT "${output_file}"
                    COMMAND ${CMAKE_COMMAND} -E make_directory "${output_folder}/Resources"
                    COMMAND ${CMAKE_COMMAND} -E copy "${full_path}" "${output_file}"
                    DEPENDS "${full_path}"
                    COMMENT "Copying and checking resource file ${file_name}"
                    )

                    list(APPEND output_files "${output_file}")
                endforeach()
                
                add_custom_target(CopyResources${target_name} ALL DEPENDS ${output_files})
                add_dependencies(CopyResources${target_name} ShaderCompiler)
                add_dependencies(${target_name} CopyResources${target_name})
            endif()

            add_dependencies(${target_name} CopyFrameworkFolder${target_name})
        endif()
        
        if(BUILD_FOR_IOS)
            set_target_properties(${target_name} PROPERTIES 
                MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/utilities/cmake/iOS/Info.plist
                XCODE_ATTRIBUTE_DEVELOPMENT_TEAM ${APPLE_SIGNING_TEAM_ID}
                XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "Apple Development"
            )
        else()
            SET(TARGET_NAME "${target_name}")

            configure_file(
                "${CMAKE_SOURCE_DIR}/utilities/cmake/MacOS/Info.plist.in"
                "${CMAKE_BINARY_DIR}/utilities/cmake/MacOS/${target_name}/Info.plist"
                @ONLY
            )
            set_target_properties(${target_name} PROPERTIES 
                MACOSX_BUNDLE_INFO_PLIST ${CMAKE_BINARY_DIR}/utilities/cmake/MacOS/${target_name}/Info.plist
            )
        endif()
    else()
        set(output_folder "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${target_name}")

        set_target_properties(${target_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${output_folder})

        add_custom_target(CopyApplicationFolder${target_name} ALL)

        add_custom_command(TARGET CopyApplicationFolder${target_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${output_folder}"
            COMMENT "Creating package folder"
        )

        foreach(dependency IN LISTS ARG_DEPENDENCIES)
            add_dependencies(CopyApplicationFolder${target_name} ${dependency})
        
            add_custom_command(TARGET CopyApplicationFolder${target_name} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${dependency}" "${output_folder}"
                COMMENT "Copy ${dependency}"
            )
        endforeach()
      
        if(NOT resources_length EQUAL 0)
            foreach(file IN LISTS ARG_RESOURCES)
                get_filename_component(file_name "${file}" NAME)
                get_filename_component(full_path "${file}" ABSOLUTE)
                get_filename_component(file_dir "${full_path}" DIRECTORY)

                set(resource_root "${CMAKE_CURRENT_BINARY_DIR}/Data") 

                file(RELATIVE_PATH relative_dir "${resource_root}" "${file_dir}")
                set(output_subdir "${output_folder}/Data/${relative_dir}")
                file(MAKE_DIRECTORY "${output_subdir}")

                set(output_file "${output_subdir}/${file_name}")
                
                add_custom_command(
                    OUTPUT "${output_file}"
                    COMMAND ${CMAKE_COMMAND} -E make_directory "${output_folder}/Data"
                    COMMAND ${CMAKE_COMMAND} -E copy "${full_path}" "${output_file}"
                    DEPENDS "${full_path}"
                    COMMENT "Copying and checking resource file ${file_name}"
                )
                
                list(APPEND output_files "${output_file}")
            endforeach()
            
            add_custom_target(CopyResources${target_name} ALL DEPENDS ${output_files})
            add_dependencies(CopyResources${target_name} ShaderCompiler)
            add_dependencies(${target_name} CopyResources${target_name})
        endif()

        add_dependencies(${target_name} CopyApplicationFolder${target_name})
    endif()

    install(DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${target_name}${APPFOLDER_EXTENSION}
        DESTINATION ${install_folder}
        USE_SOURCE_PERMISSIONS
    )
endfunction()

