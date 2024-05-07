add_executable(ApplicationTests UnityBuild.cpp)
target_link_libraries(ApplicationTests PRIVATE utest)
target_link_libraries(ApplicationTests PRIVATE ElementalInterface)

install(TARGETS ApplicationTests
    RUNTIME DESTINATION tests
    PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_EXECUTE WORLD_READ)
