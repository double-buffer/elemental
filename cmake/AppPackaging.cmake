function(configure_package_resources target_name output_folder resource_folder)
    if(NOT ARGN)
        return()
    endif()

    set(output_files "")

    foreach(file IN LISTS ARGN)
        get_filename_component(file_name "${file}" NAME)
        get_filename_component(full_path "${file}" ABSOLUTE)
        get_filename_component(file_dir "${full_path}" DIRECTORY)

        set(resource_root "${CMAKE_CURRENT_BINARY_DIR}/Data")
        file(RELATIVE_PATH relative_dir "${resource_root}" "${file_dir}")

        set(output_subdir "${output_folder}/${resource_folder}/${relative_dir}")
        set(output_file "${output_subdir}/${file_name}")

        add_custom_command(
            OUTPUT "${output_file}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${output_subdir}"
            COMMAND ${CMAKE_COMMAND} -E copy "${full_path}" "${output_file}"
            DEPENDS "${full_path}"
            COMMENT "Copying and checking resource file ${file_name}"
        )

        list(APPEND output_files "${output_file}")
    endforeach()

    add_custom_target(CopyResources${target_name} ALL DEPENDS ${output_files})
    add_dependencies(${target_name} CopyResources${target_name})
endfunction()

function(configure_xcode_resources target_name)
    foreach(file IN LISTS ARGN)
        get_filename_component(full_path "${file}" ABSOLUTE)
        get_filename_component(file_dir "${full_path}" DIRECTORY)

        set(resource_root "${CMAKE_CURRENT_BINARY_DIR}/Data")
        file(RELATIVE_PATH relative_dir "${resource_root}" "${file_dir}")

        set_source_files_properties("${full_path}"
            PROPERTIES MACOSX_PACKAGE_LOCATION "${relative_dir}"
        )

        target_sources(${target_name} PRIVATE "${full_path}")
    endforeach()
endfunction()

function(configure_framework_dependencies target_name)
    add_custom_target(CopyFrameworkFolder${target_name} ALL)

    foreach(dependency IN LISTS ARGN)
        add_dependencies(CopyFrameworkFolder${target_name} ${dependency})

        set(framework_path "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${dependency}.framework")
        get_filename_component(framework_name "${framework_path}" NAME)
        set(framework_destination "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${target_name}.app/Contents/Frameworks/${framework_name}")

        add_custom_command(TARGET CopyFrameworkFolder${target_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${framework_destination}"
            COMMAND cp -RP "${framework_path}/" "${framework_destination}/"
            COMMAND ${CMAKE_COMMAND} -E remove_directory "${framework_destination}/Headers"
            COMMENT "Copying ${dependency} framework folder to destination"
        )
    endforeach()

    add_dependencies(${target_name} CopyFrameworkFolder${target_name})
endfunction()

function(configure_directory_dependencies target_name output_folder)
    add_custom_target(CopyApplicationFolder${target_name} ALL)

    add_custom_command(TARGET CopyApplicationFolder${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${output_folder}"
        COMMENT "Creating package folder"
    )

    foreach(dependency IN LISTS ARGN)
        add_dependencies(CopyApplicationFolder${target_name} ${dependency})

        add_custom_command(TARGET CopyApplicationFolder${target_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${dependency}" "${output_folder}"
            COMMENT "Copy ${dependency}"
        )
    endforeach()

    add_dependencies(${target_name} CopyApplicationFolder${target_name})
endfunction()

function(configure_project_package target_name install_folder)
    cmake_parse_arguments(ARG "" "" "DEPENDENCIES;RESOURCES" ${ARGV})

    if(APPLE)
        set(package_path "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${target_name}.app")

        set_target_properties(${target_name} PROPERTIES
            MACOSX_BUNDLE "TRUE"
        )

        if(CMAKE_GENERATOR STREQUAL "Xcode")
            set(elemental_path "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug/Elemental.framework")

            set_target_properties(${target_name} PROPERTIES
                XCODE_EMBED_FRAMEWORKS ${elemental_path}
                XCODE_EMBED_FRAMEWORKS_CODE_SIGN_ON_COPY "YES"
                XCODE_EMBED_FRAMEWORKS_REMOVE_HEADERS_ON_COPY "YES"
            )

            configure_xcode_resources(${target_name} ${ARG_RESOURCES})
        else()
            configure_framework_dependencies(${target_name} ${ARG_DEPENDENCIES})
            configure_package_resources(
                ${target_name}
                "${package_path}/Contents"
                "Resources"
                ${ARG_RESOURCES}
            )
        endif()

        if(BUILD_FOR_IOS)
            set_target_properties(${target_name} PROPERTIES
                MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/cmake/iOS/Info.plist
                XCODE_ATTRIBUTE_DEVELOPMENT_TEAM ${APPLE_SIGNING_TEAM_ID}
                XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "Apple Development"
            )
        else()
            set(TARGET_NAME "${target_name}")

            configure_file(
                "${CMAKE_SOURCE_DIR}/cmake/MacOS/Info.plist.in"
                "${CMAKE_BINARY_DIR}/cmake/MacOS/${target_name}/Info.plist"
                @ONLY
            )

            set_target_properties(${target_name} PROPERTIES
                MACOSX_BUNDLE_INFO_PLIST ${CMAKE_BINARY_DIR}/cmake/MacOS/${target_name}/Info.plist
            )
        endif()
    else()
        set(package_path "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${target_name}")

        set_target_properties(${target_name} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${package_path}"
        )

        configure_directory_dependencies(${target_name} "${package_path}" ${ARG_DEPENDENCIES})
        configure_package_resources(${target_name} "${package_path}" "Data" ${ARG_RESOURCES})
    endif()

    install(DIRECTORY "${package_path}"
        DESTINATION ${install_folder}
        USE_SOURCE_PERMISSIONS
    )
endfunction()
