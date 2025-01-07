if(WIN32)
    set(DIRECT3D12_VERSION "1.614.0")

    add_library(Direct3D12 INTERFACE)
    
    download_and_extract_nuget_package(Direct3D12 "Microsoft.Direct3D.D3D12" ${DIRECT3D12_VERSION})
    target_link_libraries(Direct3D12 INTERFACE dxgi dxguid)

    # TODO: Do we really need that?
    target_link_libraries(Direct3D12 INTERFACE d3d12)

    function(copy_direct3d12_to_target target)
        # TODO: To Fix
        set(DIRECT3D12_VERSION "1.614.0")
        file(GLOB DIRECT3D12_DLLS 
             "${CMAKE_BINARY_DIR}/external/DirectX12/packages/Microsoft.Direct3D.D3D12_${DIRECT3D12_VERSION}/build/native/bin/x64/*.dll")
        foreach(DLL ${DIRECT3D12_DLLS})
            get_filename_component(DLL_NAME "${DLL}" NAME)
            add_custom_command(TARGET ${target} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${DLL}"
                    "$<TARGET_FILE_DIR:${target}>/${DLL_NAME}")
        endforeach()
    endfunction()
endif()
