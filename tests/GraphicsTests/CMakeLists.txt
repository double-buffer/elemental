add_executable(GraphicsTests UnityBuild.cpp)
target_link_libraries(GraphicsTests PRIVATE utest)
target_link_libraries(GraphicsTests PRIVATE ElementalInterface)

install(TARGETS GraphicsTests
    RUNTIME DESTINATION tests
    PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_EXECUTE WORLD_READ)
