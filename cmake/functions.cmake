function(extract_zip_file pathZip pathExtract)
    if (UNIX)
        # Use unzip command on Unix-based systems
        execute_process(
            COMMAND unzip -q ${pathZip} -d ${pathExtract}
        )
    elseif (WIN32)
        # Use powershell command on Windows systems
        execute_process(
            COMMAND powershell -Command "Expand-Archive -Force -Path '${pathZip}' -DestinationPath '${pathExtract}'"
        )
    endif()
endfunction()

function(get_github_release repo tag filenamePattern pathExtract)
    # Check if the output directory already exists
    if (EXISTS ${pathExtract})
        message(STATUS "Output directory ${pathExtract} already exists, skipping download.")
        return()
    else()
        file(MAKE_DIRECTORY ${pathExtract})
    endif()

    message(STATUS "Downloading ${repo}")

    # Set the authorization headers if the GITHUB_TOKEN environment variable is defined
    if (DEFINED ENV{GITHUB_TOKEN})
        set(headers "Authorization=Bearer $ENV{GITHUB_TOKEN}")
    else()
        set(headers "")
    endif()

    # Get the download URL of the asset with the given filename pattern from the specified GitHub repository and tag
    set(releasesUri "https://api.github.com/repos/${repo}/releases/tags/${tag}")
    execute_process(
        COMMAND curl -s -H "Accept: application/vnd.github+json" ${headers} ${releasesUri}
        OUTPUT_VARIABLE releasesJson
    )

    # Convert filenamePattern to regex pattern
    string(REGEX REPLACE "\\." "\\\\." filenamePattern "${filenamePattern}")
    string(REGEX REPLACE "\\*" ".*" filenamePattern "${filenamePattern}")
    string(REGEX REPLACE "\\?" "." filenamePattern "${filenamePattern}")

    # Extract the download URL of the asset from the releases JSON
    string(REGEX MATCH "\"browser_download_url\": \"[^\"]*${filenamePattern}\"" downloadUrlMatch "${releasesJson}")
    string(REGEX REPLACE "\"browser_download_url\": \"" "" downloadUrl "${downloadUrlMatch}")
    string(REGEX REPLACE "\".*" "" downloadUrl "${downloadUrl}")

    # Download the asset to a temporary zip file
    set(pathZip "${CMAKE_BINARY_DIR}/temp.zip")
    execute_process(
        COMMAND curl -s -L -o ${pathZip} ${downloadUrl}
    )

    # Extract the downloaded zip file to the extractPath
    extract_zip_file(${pathZip} ${pathExtract})

    # Clean up the temporary zip file
    file(REMOVE ${pathZip})

endfunction()

function(download_and_extract_nuget_package target packageId packageVersion)
    download_and_extract_nuget_package_path(${target} ${packageId} ${packageVersion} "" "" "")
endfunction()

function(download_and_extract_nuget_package_path target packageId packageVersion includePath libPath binPath)
    # Create the download URL
    set(DOWNLOAD_URL "https://www.nuget.org/api/v2/package/${packageId}/${packageVersion}")

    # Set the output directories for the downloaded package
    set(PACKAGE_DIR "${CMAKE_CURRENT_BINARY_DIR}/packages/${packageId}_${packageVersion}")
    set(PACKAGE_ZIP "${packageId}_${packageVersion}.zip")
    set(DEFAULT_INCLUDE_DIR "build/native/include")
    set(DEFAULT_LIB_DIR "build/native/lib/${CMAKE_SYSTEM_PROCESSOR}")
    set(DEFAULT_BIN_DIR "build/native/bin/${CMAKE_SYSTEM_PROCESSOR}")

    # Use the custom include and lib paths if provided, otherwise use the default paths
    if(includePath)
        set(INCLUDE_DIR "${PACKAGE_DIR}/${includePath}")
    else()
        set(INCLUDE_DIR "${PACKAGE_DIR}/${DEFAULT_INCLUDE_DIR}")
    endif()

    if(libPath)
        set(LIB_DIR "${PACKAGE_DIR}/${libPath}")
    else()
        set(LIB_DIR "${PACKAGE_DIR}/${DEFAULT_LIB_DIR}")
    endif()

    if(binPath)
        set(BIN_DIR "${PACKAGE_DIR}/${binPath}")
    else()
        set(BIN_DIR "${PACKAGE_DIR}/${DEFAULT_BIN_DIR}")
    endif()
    
    # Check if the output directory already exists
    if (EXISTS ${PACKAGE_DIR})
        message(STATUS "Package directory ${PACKAGE_DIR} already exists, skipping download.")
    else()
        # Create the directories if they don't exist
        file(MAKE_DIRECTORY ${PACKAGE_DIR})

        # Download the NuGet package
        file(DOWNLOAD "${DOWNLOAD_URL}" "${PACKAGE_DIR}/${PACKAGE_ZIP}")

        # Extract the package contents
        extract_zip_file("${PACKAGE_DIR}/${PACKAGE_ZIP}" "${PACKAGE_DIR}")
        
        # Clean up the temporary zip file
        file(REMOVE "${PACKAGE_DIR}/${PACKAGE_ZIP}")

        message(STATUS "Downloaded NuGet package to: ${PACKAGE_DIR}")
    endif()

    # Set the include and lib directories for the target
    if (EXISTS ${INCLUDE_DIR})
        target_include_directories(${target} INTERFACE ${INCLUDE_DIR})
        message(STATUS "Include directory: ${INCLUDE_DIR}")
    endif ()
    
    if (EXISTS ${LIB_DIR})
        target_link_directories(${target} INTERFACE ${LIB_DIR})
        message(STATUS "Lib directory: ${LIB_DIR}")
    endif()
    
    if (EXISTS ${BIN_DIR})
        target_link_directories(${target} INTERFACE ${BIN_DIR})

        message(STATUS "Bin directory: ${BIN_DIR}")
    endif()
    
endfunction()