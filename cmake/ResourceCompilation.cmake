function(get_host_compiler_bin_path binary_name out_var)
    set(bin_path "${CMAKE_SOURCE_DIR}/build/bin/${binary_name}.app/Contents/MacOS/${binary_name}")
    set(${out_var} "${bin_path}" PARENT_SCOPE)
endfunction()

function(configure_resources_for_compiler
    target_name
    result_var
    name
    binary_name
    source_exts
    dest_ext)

    string(REPLACE "," ";" source_exts "${source_exts}")

    get_target_property(target_sources ${target_name} SOURCES)

    set(resource_files "")
    foreach(src IN LISTS target_sources)
        set(abs_src "${CMAKE_CURRENT_SOURCE_DIR}/${src}")

        if(IS_DIRECTORY "${abs_src}")
            foreach(ext IN LISTS source_exts)
                file(GLOB_RECURSE matched_files "${abs_src}/*${ext}")
                list(APPEND resource_files ${matched_files})
            endforeach()
        else()
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

    if(BUILD_FOR_IOS)
        get_host_compiler_bin_path(${binary_name} compiler_command)
        set(compiler_dependencies "")
    else()
        set(compiler_command "$<TARGET_FILE:${binary_name}>")
        set(compiler_dependencies ${binary_name})
    endif()

    set(default_params "")
    foreach(param IN LISTS ARGN)
        if(NOT param STREQUAL "")
            list(APPEND default_params ${param})
        endif()
    endforeach()

    set(compiled_files "")

    foreach(file IN LISTS resource_files)
        get_filename_component(file_path "${file}" ABSOLUTE)
        get_filename_component(file_dir "${file_path}" DIRECTORY)
        get_filename_component(file_name "${file_path}" NAME_WE)

        file(RELATIVE_PATH relative_dir "${CMAKE_CURRENT_SOURCE_DIR}" "${file_dir}")

        set(output_dir "${CMAKE_CURRENT_BINARY_DIR}/${relative_dir}")
        file(MAKE_DIRECTORY "${output_dir}")

        set(output_file "${output_dir}/${file_name}${dest_ext}")

        add_custom_command(
            OUTPUT "${output_file}"
            COMMAND "${compiler_command}" ${default_params} "${file}" "${output_file}"
            DEPENDS ${compiler_dependencies} "${file}"
            COMMENT "Compiling ${name}: ${file}"
            WORKING_DIRECTORY "${file_dir}"
        )

        set_source_files_properties("${output_file}" PROPERTIES GENERATED TRUE)
        target_sources("${target_name}" PRIVATE "${output_file}")
        list(APPEND compiled_files "${output_file}")

        if("${name}" STREQUAL "HLSL" AND WIN32)
            set(vulkan_output_file "${output_dir}/${file_name}_vulkan${dest_ext}")
            add_custom_command(
                OUTPUT "${vulkan_output_file}"
                COMMAND "${compiler_command}" ${default_params} --target-api vulkan "${file}" "${vulkan_output_file}"
                DEPENDS ${compiler_dependencies} "${file}"
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

function(configure_resource_compilation target_name result_var)
    cmake_parse_arguments(RESOURCE "" "MESH_COMPILER;MESH_OUTPUT_EXTENSION" "" ${ARGN})

    if(RESOURCE_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unknown resource compilation arguments: ${RESOURCE_UNPARSED_ARGUMENTS}")
    endif()

    if(NOT RESOURCE_MESH_COMPILER)
        set(RESOURCE_MESH_COMPILER SceneCompiler)
    endif()

    if(NOT RESOURCE_MESH_OUTPUT_EXTENSION)
        set(RESOURCE_MESH_OUTPUT_EXTENSION .scene)
    endif()

    if(BUILD_FOR_IOS)
        set(SHADER_COMPILER_DEFAULT_OPTIONS "--target-platform iOS")
    else()
        set(SHADER_COMPILER_DEFAULT_OPTIONS "")
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

    set(COMPILERS_LIST
        "HLSL|ShaderCompiler|.hlsl|.shader|${SHADER_COMPILER_DEFAULT_OPTIONS}"
        "MESH|${RESOURCE_MESH_COMPILER}|.obj|${RESOURCE_MESH_OUTPUT_EXTENSION}|${MESH_COMPILER_DEFAULT_OPTIONS}"
        "MESH|${RESOURCE_MESH_COMPILER}|.gltf|${RESOURCE_MESH_OUTPUT_EXTENSION}|${MESH_COMPILER_DEFAULT_OPTIONS}"
        "TEXTURE|TextureCompiler|.tga|.texture|${TEXTURE_COMPILER_DEFAULT_OPTIONS}"
        "TEXTURE|TextureCompiler|.jpg|.texture|${TEXTURE_COMPILER_DEFAULT_OPTIONS}"
        "TEXTURE|TextureCompiler|.png|.texture|${TEXTURE_COMPILER_DEFAULT_OPTIONS}"
        "TEXTURE|TextureCompiler|.dds|.texture|${TEXTURE_COMPILER_DEFAULT_OPTIONS}"
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

        configure_resources_for_compiler(
            "${target_name}"
            compiled_files_for_${name}
            "${name}"
            "${binary_name}"
            "${source_exts}"
            "${dest_ext}"
            ${default_params}
        )

        list(APPEND all_compiled_resources ${compiled_files_for_${name}})
    endforeach()

    set(${result_var} "${all_compiled_resources}" PARENT_SCOPE)
endfunction()
