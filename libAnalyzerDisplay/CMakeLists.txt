
if(ROOT_FOUND)
    add_executable(display_example display_example.cxx)
    target_link_libraries(display_example PUBLIC rootana)
endif()
