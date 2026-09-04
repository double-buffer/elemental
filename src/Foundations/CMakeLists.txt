if(WIN32)
    set(FOUNDATIONS_PLATFORM_DIR Microsoft)
elseif(APPLE)
    set(FOUNDATIONS_PLATFORM_DIR Apple)
elseif(LINUX)
    set(FOUNDATIONS_PLATFORM_DIR Linux)
endif()

add_library(foundations INTERFACE)

target_include_directories(foundations INTERFACE
    .
    ${FOUNDATIONS_PLATFORM_DIR}
)
