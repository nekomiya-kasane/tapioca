# ── Test targets for tapioca ──────────────────────────────────────────────

function(tapioca_add_test name)
    add_executable(${name} ${ARGN})
    target_link_libraries(${name} PRIVATE tapioca gtest gtest_main)
    set_target_properties(${name} PROPERTIES
        CXX_STANDARD 23
        CXX_STANDARD_REQUIRED ON
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/code/bin"
    )
    add_test(NAME ${name} COMMAND ${name})

    # Copy tapioca.dll + icetea.dll next to test exe (Windows)
    if(WIN32)
        foreach(_lib tapioca icetea)
            if(TARGET ${_lib})
                add_custom_command(TARGET ${name} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        $<TARGET_FILE:${_lib}> $<TARGET_FILE_DIR:${name}>
                    COMMENT "[${name}] Copying ${_lib}.dll"
                )
            endif()
        endforeach()
    endif()
endfunction()

tapioca_add_test(tapioca_style_test          style_test.cpp)
tapioca_add_test(tapioca_color_test          color_test.cpp)
tapioca_add_test(tapioca_canvas_test         canvas_test.cpp)
tapioca_add_test(tapioca_layout_types_test   layout_types_test.cpp)
tapioca_add_test(tapioca_ansi_test           ansi_test.cpp)
tapioca_add_test(tapioca_unicode_width_test  unicode_width_test.cpp)
tapioca_add_test(tapioca_image_test          image_test.cpp)
tapioca_add_test(tapioca_console_pal_test    console_pal_test.cpp)
tapioca_add_test(tapioca_color_downgrade_test color_downgrade_test.cpp)
