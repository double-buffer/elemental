add_executable(ToolsTests UnityBuild.cpp)
target_link_libraries(ToolsTests PRIVATE utest)
target_link_libraries(ToolsTests PRIVATE ElementalToolsInterface)

install(TARGETS ToolsTests
    RUNTIME DESTINATION tests
    PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_EXECUTE WORLD_READ)
