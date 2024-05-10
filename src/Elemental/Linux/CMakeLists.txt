target_compile_options(Elemental PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wno-nullability-extension>)
target_compile_options(Elemental PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wno-c11-extensions>)

find_package(PkgConfig REQUIRED)

pkg_check_modules(GTK4 REQUIRED gtk4)

target_include_directories(Elemental PRIVATE ${GTK4_INCLUDE_DIRS})
target_link_directories(Elemental PRIVATE ${GTK4_LIBRARY_DIRS})
target_compile_options(Elemental PRIVATE ${GTK4_CFLAGS_OTHER})
target_link_libraries(Elemental PRIVATE ${GTK4_LIBRARIES})

set_target_properties(Elemental PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Elemental")
set(ELEMENTAL_OUTPUT_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Elemental/" PARENT_SCOPE)

function(copy_platform_dependencies target)
endfunction()
