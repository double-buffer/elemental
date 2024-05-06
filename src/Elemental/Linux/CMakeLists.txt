target_compile_options(Elemental PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wno-nullability-extension>)
target_compile_options(Elemental PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wno-c11-extensions>)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK4 REQUIRED gtk4)

target_include_directories(Elemental PRIVATE ${GTK4_INCLUDE_DIRS})
target_link_directories(Elemental PRIVATE ${GTK4_LIBRARY_DIRS})
target_compile_options(Elemental PRIVATE ${GTK4_CFLAGS_OTHER})
target_link_libraries(Elemental PRIVATE ${GTK4_LIBRARIES})

pkg_check_modules(GLIB REQUIRED glib-2.0)

target_include_directories(Elemental PRIVATE ${GLIB_INCLUDE_DIRS})
target_link_directories(Elemental PRIVATE ${GLIB_LIBRARY_DIRS})
target_compile_options(Elemental PRIVATE ${GLIB_CFLAGS_OTHER})
target_link_libraries(Elemental PRIVATE ${GLIB_LIBRARIES})

function(copy_platform_dependencies target)
endfunction()
