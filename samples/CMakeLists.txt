add_library(SampleCommon INTERFACE)
target_include_directories(SampleCommon INTERFACE ./Common/)
target_link_libraries(SampleCommon INTERFACE ElementalInterface)
