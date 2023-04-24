
function(get_github_release repo tag filenamePattern pathExtract)
    # Check if the output directory already exists
    if (EXISTS ${pathExtract})
        message("Output directory ${pathExtract} already exists, skipping download.")
        return()
    else()
        file(MAKE_DIRECTORY ${pathExtract})
    endif()

    message("Downloading ${repo}")

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

    # Clean up the temporary zip file
    file(REMOVE ${pathZip})

endfunction()