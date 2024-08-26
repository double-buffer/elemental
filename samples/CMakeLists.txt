add_library(SampleCommon INTERFACE)
target_link_libraries(SampleCommon INTERFACE cimgui)
target_include_directories(SampleCommon INTERFACE ./Common/)
