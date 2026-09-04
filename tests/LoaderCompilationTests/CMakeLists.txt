if(BUILD_FOR_IOS)
    return()
endif()

add_executable(
    LoaderCompilationTests
    main.c
    ElementalTranslationUnit1.c
    ElementalTranslationUnit2.c
    ElementalToolsTranslationUnit1.c
    ElementalToolsTranslationUnit2.c
)

target_link_libraries(LoaderCompilationTests PRIVATE ElementalInterface ElementalToolsInterface)
